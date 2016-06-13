/*
* outputAPI.c
*
*      Author: Colleen Barr
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "outputAPI.h"
#include "datetime.h"

#define MEMCHECK(x)  (((x) == NULL) ? 411 : 0 )

#define ULLCAST long long // search and replace

static const int RECORDSIZE = 4;       // number of bytes per file record

//-----------------------------------------------------------------------------
//  Shared variables
//-----------------------------------------------------------------------------

struct SMOutputAPI {
	char name[MAXFNAME + 1];           // file path/name
	bool isOpened;                     // current state (CLOSED = 0, OPEN = 1)
	FILE* file;                        // FILE structure pointer

	long Nperiods;                      // number of reporting periods
	int FlowUnits;                     // flow units code

	int Nsubcatch;                     // number of subcatchments
	int Nnodes;                        // number of drainage system nodes
	int Nlinks;                        // number of drainage system links
	int Npolluts;                      // number of pollutants tracked

	int SubcatchVars;                  // number of subcatch reporting variables
	int NodeVars;                      // number of node reporting variables
	int LinkVars;                      // number of link reporting variables
	int SysVars;                       // number of system reporting variables

	double StartDate;                  // start date of simulation
	int    ReportStep;                 // reporting time step (seconds)

	ULLCAST IDPos;					       // file position where object ID names start
	ULLCAST ObjPropPos;					   // file position where object properties start
	ULLCAST ResultsPos;                    // file position where results start
	ULLCAST BytesPerPeriod;                // bytes used for results in each period
};

//-----------------------------------------------------------------------------
//   Local functions
//-----------------------------------------------------------------------------
double DLLEXPORT getTimeValue(SMOutputAPI* smoapi, long timeIndex);
float DLLEXPORT getSubcatchValue(SMOutputAPI* smoapi, long timeIndex, int subcatchIndex,
	SMO_subcatchAttribute attr);
float DLLEXPORT getNodeValue(SMOutputAPI* smoapi, long timeIndex, int nodeIndex, SMO_nodeAttribute attr);
float DLLEXPORT getLinkValue(SMOutputAPI* smoapi, long timeIndex, int linkIndex, SMO_linkAttribute attr);
float DLLEXPORT getSystemValue(SMOutputAPI* smoapi, long timeIndex, SMO_systemAttribute attr);

void AddIDentry(struct IDentry* head, char* idname, int numChar);

int DLLEXPORT SMR_open(const char* path, SMOutputAPI** smoapi)
//
//  Purpose: Open the output binary file and read epilogue.
//
{
	int magic1, magic2, errCode, version;
	ULLCAST offset;
	
	int err;
	
	*smoapi = malloc(sizeof(SMOutputAPI));
	
	strncpy((*smoapi)->name, path, MAXFNAME);
	(*smoapi)->isOpened = false;

	// --- open the output file
	if (((*smoapi)->file = fopen(path, "rb")) == NULL)
		return 434;
	else
		(*smoapi)->isOpened = true;
	
	// --- check that file contains at least 14 records
	fseeko((*smoapi)->file, 0L, SEEK_END);
	if (ftello((*smoapi)->file) < 14 * RECORDSIZE) {
		fclose((*smoapi)->file);
		return 435;
	}
	
	// --- read parameters from end of file
	fseeko((*smoapi)->file, -6 * RECORDSIZE, SEEK_END);
	fread(&((*smoapi)->IDPos), RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->ObjPropPos), RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->ResultsPos), RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->Nperiods), RECORDSIZE, 1, (*smoapi)->file);
	fread(&errCode, RECORDSIZE, 1, (*smoapi)->file);
	fread(&magic2, RECORDSIZE, 1, (*smoapi)->file);

	// --- read magic number from beginning of file
	fseeko((*smoapi)->file, 0L, SEEK_SET);
	fread(&magic1, RECORDSIZE, 1, (*smoapi)->file);

	// --- perform error checks
	if (magic1 != magic2) err = 435;
	else if (errCode != 0) err = 435;
	else if ((*smoapi)->Nperiods == 0) err = 435;
	else err = 0;

	// --- quit if errors found
	if (err > 0)
	{
		fclose((*smoapi)->file);
		(*smoapi)->file = NULL;
		return err;
	}

	// --- otherwise read additional parameters from start of file
	fread(&version, RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->FlowUnits), RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->Nsubcatch), RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->Nnodes), RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->Nlinks), RECORDSIZE, 1, (*smoapi)->file);
	fread(&((*smoapi)->Npolluts), RECORDSIZE, 1, (*smoapi)->file);

	// Skip over saved subcatch/node/link input values
		offset = (ULLCAST)((ULLCAST)(*smoapi)->Nsubcatch + (ULLCAST)2) * (ULLCAST)RECORDSIZE  // Subcatchment area
		+ (ULLCAST)((ULLCAST)3 * (ULLCAST)(*smoapi)->Nnodes + (ULLCAST)4) * (ULLCAST)RECORDSIZE  // Node type, invert & max depth
		+ (ULLCAST)((ULLCAST)5 * (ULLCAST)(*smoapi)->Nlinks + (ULLCAST)6) * (ULLCAST)RECORDSIZE; // Link type, z1, z2, max depth & length
	offset = (ULLCAST)(*smoapi)->ObjPropPos + (ULLCAST)offset;
	fseeko((*smoapi)->file, offset, SEEK_SET);

	// Read number & codes of computed variables
	fread(&((*smoapi)->SubcatchVars), RECORDSIZE, 1, (*smoapi)->file); // # Subcatch variables
	fseeko((*smoapi)->file, (*smoapi)->SubcatchVars*RECORDSIZE, SEEK_CUR);
	fread(&((*smoapi)->NodeVars), RECORDSIZE, 1, (*smoapi)->file);     // # Node variables
	fseeko((*smoapi)->file, (*smoapi)->NodeVars*RECORDSIZE, SEEK_CUR);
	fread(&((*smoapi)->LinkVars), RECORDSIZE, 1, (*smoapi)->file);     // # Link variables
	fseeko((*smoapi)->file, (*smoapi)->LinkVars*RECORDSIZE, SEEK_CUR);
	fread(&((*smoapi)->SysVars), RECORDSIZE, 1, (*smoapi)->file);     // # System variables

	// --- read data just before start of output results
	offset = (ULLCAST)(*smoapi)->ResultsPos - 3LL * (ULLCAST)RECORDSIZE;
	fseeko((*smoapi)->file, offset, SEEK_SET);
	fread(&((*smoapi)->StartDate), sizeof(double), 1, (*smoapi)->file);
	fread(&((*smoapi)->ReportStep), RECORDSIZE, 1, (*smoapi)->file);

	// --- compute number of bytes of results values used per time period
	(*smoapi)->BytesPerPeriod = (ULLCAST)2 * (ULLCAST)RECORDSIZE +      // date value (a double)
		((ULLCAST)(*smoapi)->Nsubcatch*(ULLCAST)(*smoapi)->SubcatchVars +
		(ULLCAST)(*smoapi)->Nnodes*(ULLCAST)(*smoapi)->NodeVars +
		(ULLCAST)(*smoapi)->Nlinks*(ULLCAST)(*smoapi)->LinkVars +
		(ULLCAST)(*smoapi)->SysVars)*(ULLCAST)RECORDSIZE;

	// --- return with file left open
	return err;

}


int DLLEXPORT SMO_getProjectSize(SMOutputAPI* smoapi, SMO_elementCount code, int* count)
//
//   Purpose: Returns project size.
// 
{
	*count = -1;
	if (smoapi->isOpened) 
	{
		switch (code)
		{
		case subcatchCount:		*count = smoapi->Nsubcatch;	break;
		case nodeCount:			*count = smoapi->Nnodes;	break;
		case linkCount:			*count = smoapi->Nlinks;	break;
		case pollutantCount:	*count = smoapi->Npolluts;	break;
		default: return 421;
		}
		return 0;
	}
	return 412;
}


int DLLEXPORT SMO_getUnits(SMOutputAPI* smoapi, SMO_unit code, int* unitFlag)
//
//   Purpose: Returns flow rate units.
//
//	 Note:    Concentration units are located after the pollutant ID names and before the object properties start,
//			  and can differ for each pollutant.  They're stored as 4-byte integers with the following codes:
//		          0: mg/L
//				  1: ug/L
//				  2: counts/L
//		      Probably the best way to do this would not be here -- instead write a function that takes 
//	          NPolluts and ObjPropPos, jump to ObjPropPos, count backward (NPolluts * 4), then read forward
//			  to get the units for each pollutant
//
{
	*unitFlag = -1;
	if (smoapi->isOpened) 
	{
		switch (code)
		{
		case flow_rate:			*unitFlag = smoapi->FlowUnits; break;
			//		case concentration:		*unitFlag = ConcUnits; break;
		default: return 421;
		}
		return 0;
	}
	return 412;
}

int DLLEXPORT SMO_getStartTime(SMOutputAPI* smoapi, double* time)
//
//	Purpose: Returns start date.
//
{
	*time = -1;
	if (smoapi->isOpened)
	{
		*time = smoapi->StartDate;
		return 0;
	}
	return 412;
}


int DLLEXPORT SMO_getTimes(SMOutputAPI* smoapi, SMO_time code, int* time)
//
//   Purpose: Returns step size and number of periods.
//
{
	*time = -1;
	if (smoapi->isOpened) 
	{
		switch (code)
		{
		case reportStep:  *time = smoapi->ReportStep;   break;
		case numPeriods:  *time = smoapi->Nperiods;     break;
		default: return 421;
		}
		return 0;
	}
	return 412;
}


void DLLEXPORT AddIDentry(struct IDentry* head, char* idname, int numChar)
//
//	Purpose: add ID to linked list (can't be used for first entry).
//
{
	idEntry* current = head;
	while (current->nextID != NULL)
	{
		current = current->nextID;
	}

	current->nextID = malloc(sizeof(idEntry));

	current->nextID->IDname = calloc(numChar + 1, sizeof(char));
	strcpy(current->nextID->IDname, idname);

	current->nextID->nextID = NULL;
}

struct DLLEXPORT IDentry* SMO_getSubcatchIDs(SMOutputAPI* smoapi, int *errcode)
//
//	 Purpose: Get subcatchment IDs. 
//
//   Warning: Caller must free memory allocated by this function using SMO_free_list
//
//	 Note:	  The number of characters of each ID can vary and is stored in the binary file before each ID
//			  No null characters or spaces are used to separate the IDs or number of characters
//
{
	int arraySize = (smoapi->Nsubcatch); 
	int* numChar = (int*)calloc(arraySize, RECORDSIZE);
	int stringSize = 0;
	int i;

	char *idname;

	idEntry* head = NULL;

	if (arraySize == 0)
	{
		free(numChar);
		*errcode = 411;
		return head;
	}

	if (smoapi->isOpened)
	{
		head = (idEntry*)malloc(sizeof(idEntry));

		rewind(smoapi->file);
		fseeko(smoapi->file, smoapi->IDPos, SEEK_SET);

		fread(&numChar[0], RECORDSIZE, 1, smoapi->file);
		idname = calloc(numChar[0] + 1, sizeof(char));
		fread(idname, sizeof(char), numChar[0], smoapi->file);

		head[0].IDname = calloc(numChar[0] + 1, sizeof(char));
		strcpy(head[0].IDname, idname);
		(*head).nextID = NULL;

		free(idname);

		for (i = 1; i < arraySize; i++)
		{
			fread(&numChar[i], RECORDSIZE, 1, smoapi->file);
			idname = calloc(numChar[i] + 1, sizeof(char));
			fread(idname, sizeof(char), numChar[i], smoapi->file);
			AddIDentry(head, idname, numChar[i]);
			free(idname);
		}

		free(numChar);

		*errcode = 0;
		return head;
	}

	*errcode = 412;
	return head;
}

struct DLLEXPORT IDentry* SMO_getNodeIDs(SMOutputAPI* smoapi, int* errcode)
//
//	 Purpose: Get node IDs. 
//
//   Warning: Caller must free memory allocated by this function using SMO_free_list
//
{
	int arraySize = (smoapi->Nnodes);
	int* numChar = (int*)calloc(arraySize, RECORDSIZE);
	int stringSize = 0;
	int i;

	char *idname;

	idEntry* head = NULL;

	// new
	int fwdSize = smoapi->Nsubcatch;
	int* fwdNumChar = (int*)calloc(fwdSize, RECORDSIZE);

	if (arraySize == 0)
	{
		free(fwdNumChar);
		free(numChar);
		*errcode = 411;
		return head;
	}

	if (smoapi->isOpened)
	{
		head = (idEntry*)malloc(sizeof(idEntry));
		rewind(smoapi->file);
		fseeko(smoapi->file, smoapi->IDPos, SEEK_SET);

		// fast forward through subcatchment IDs
		for (i = 0; i < fwdSize; i++)
		{
			fread(&fwdNumChar[i], RECORDSIZE, 1, smoapi->file);
			fseeko(smoapi->file, fwdNumChar[i], SEEK_CUR);
		}

		fread(&numChar[0], RECORDSIZE, 1, smoapi->file);
		idname = calloc(numChar[0] + 1, sizeof(char));
		fread(idname, sizeof(char), numChar[0], smoapi->file);

		head[0].IDname = calloc(numChar[0] + 1, sizeof(char));
		strcpy(head[0].IDname, idname);
		(*head).nextID = NULL;

		free(idname);

		for (i = 1; i < arraySize; i++)
		{
			fread(&numChar[i], RECORDSIZE, 1, smoapi->file);
			idname = calloc(numChar[i] + 1, sizeof(char));
			fread(idname, sizeof(char), numChar[i], smoapi->file);
			AddIDentry(head, idname, numChar[i]);
			free(idname);
		}

		free(fwdNumChar);
		free(numChar);

		*errcode = 0;
		return head;
	}

	*errcode = 412;
	return head;
}

struct IDentry* SMO_getLinkIDs(SMOutputAPI* smoapi, int* errcode)
//
//	 Purpose: Get link IDs. 
//
//   Warning: Caller must free memory allocated by this function using SMO_free_list
//
{
	int arraySize = (smoapi->Nlinks);
	int* numChar = (int*)calloc(arraySize, RECORDSIZE);
	int stringSize = 0;
	int i;

	char *idname;

	idEntry* head = NULL;

	// new
	int fwdSize = smoapi->Nsubcatch + smoapi->Nnodes;
	int* fwdNumChar = (int*)calloc(fwdSize, RECORDSIZE);

	if (arraySize == 0)
	{
		free(fwdNumChar);
		free(numChar);
		*errcode = 411;
		return head;
	}

	if (smoapi->isOpened)
	{
		head = (idEntry*)malloc(sizeof(idEntry));
		rewind(smoapi->file);
		fseeko(smoapi->file, smoapi->IDPos, SEEK_SET);

		// fast forward through subcatchment and node IDs
		for (i = 0; i < fwdSize; i++)
		{
			fread(&fwdNumChar[i], RECORDSIZE, 1, smoapi->file);
			fseeko(smoapi->file, fwdNumChar[i], SEEK_CUR);
		}

		fread(&numChar[0], RECORDSIZE, 1, smoapi->file);
		idname = calloc(numChar[0] + 1, sizeof(char));
		fread(idname, sizeof(char), numChar[0], smoapi->file);

		head[0].IDname = calloc(numChar[0] + 1, sizeof(char));
		strcpy(head[0].IDname, idname);
		(*head).nextID = NULL;

		free(idname);

		for (i = 1; i < arraySize; i++)
		{
			fread(&numChar[i], RECORDSIZE, 1, smoapi->file);
			idname = calloc(numChar[i] + 1, sizeof(char));
			fread(idname, sizeof(char), numChar[i], smoapi->file);
			AddIDentry(head, idname, numChar[i]);
			free(idname);
		}

		free(fwdNumChar);
		free(numChar);

		*errcode = 0;
		return head;
	}

	*errcode = 412;
	return head;
}


struct IDentry* SMO_getPollutIDs(SMOutputAPI* smoapi, int* errcode)
//
//	 Purpose: Get pollutant IDs. 
//
//   Warning: Caller must free memory allocated by this function using SMO_free_list
//
{
	int arraySize = (smoapi->Npolluts);
	int* numChar = (int*)calloc(arraySize, RECORDSIZE);
	int stringSize = 0;
	int i;

	char *idname;

	idEntry* head = NULL;

	// new
	int fwdSize = smoapi->Nsubcatch + smoapi->Nnodes + smoapi->Nlinks;
	int* fwdNumChar = (int*)calloc(fwdSize, RECORDSIZE);

	if (arraySize == 0)
	{
		free(fwdNumChar);
		free(numChar);
		*errcode = 411;
		return head;
	}

	if (smoapi->isOpened)
	{
		head = (idEntry*)malloc(sizeof(idEntry));

		rewind(smoapi->file);
		fseeko(smoapi->file, smoapi->IDPos, SEEK_SET);

		// fast forward through subcatchment, node, and link IDs
		for (i = 0; i < fwdSize; i++)
		{
			fread(&fwdNumChar[i], RECORDSIZE, 1, smoapi->file);
			fseeko(smoapi->file, fwdNumChar[i], SEEK_CUR);
		}

		fread(&numChar[0], RECORDSIZE, 1, smoapi->file);
		idname = calloc(numChar[0] + 1, sizeof(char));
		fread(idname, sizeof(char), numChar[0], smoapi->file);

		head[0].IDname = calloc(numChar[0] + 1, sizeof(char));
		strcpy(head[0].IDname, idname);
		(*head).nextID = NULL;

		free(idname);

		for (i = 1; i < arraySize; i++)
		{
			fread(&numChar[i], RECORDSIZE, 1, smoapi->file);
			idname = calloc(numChar[i] + 1, sizeof(char));
			fread(idname, sizeof(char), numChar[i], smoapi->file);
			AddIDentry(head, idname, numChar[i]);
			free(idname);
		}

		free(fwdNumChar);
		free(numChar);

		*errcode = 0;
		return head;
	}

	*errcode = 412;
	return head;
}




float* DLLEXPORT SMO_newOutValueSeries(SMOutputAPI* smoapi, long seriesStart,
	long seriesLength, long* length, int* errcode)
//
//  Purpose: Allocates memory for outValue Series.
//
//  Warning: Caller must free memory allocated by this function using SMO_free().
//
{
	long size;
	float* array;

	if (smoapi->isOpened) 
	{
		size = seriesLength - seriesStart;
		if (size > smoapi->Nperiods)
			size = smoapi->Nperiods;

		array = (float*)calloc(size, sizeof(float));
		*errcode = (MEMCHECK(array));

		*length = size;
		return array;
	}
	*errcode = 412;
	return NULL;
}


float* DLLEXPORT SMO_newOutValueArray(SMOutputAPI* smoapi, SMO_apiFunction func,
	SMO_elementType type, long* length, int* errcode)
//
// Purpose: Allocates memory for outValue Array.
//
//  Warning: Caller must free memory allocated by this function using SMO_free().
//
{
	long size;
	float* array;

	if (smoapi->isOpened) 
	{
		switch (func)
		{
		case getAttribute:
			if (type == subcatch)
				size = smoapi->Nsubcatch;
			else if (type == node)
				size = smoapi->Nnodes;
			else if (type == link)
				size = smoapi->Nlinks;
			else // system
				size = 1;
		break;

		case getResult:
			if (type == subcatch)
				size = smoapi->SubcatchVars;
			else if (type == node)
				size = smoapi->NodeVars;
			else if (type == link)
				size = smoapi->LinkVars;
			else // system
				size = smoapi->SysVars;
		break;

		default: *errcode = 421;
			return NULL;
		}

		// Allocate memory for outValues
		array = (float*)calloc(size, sizeof(float));
		*errcode = (MEMCHECK(array));

		*length = size;
		return array;
	}
	*errcode = 412;
	return NULL;
}


double* DLLEXPORT SMO_newOutTimeList(SMOutputAPI* smoapi, int* errcode)
//
//  Purpose: Allocates memory for TimeList.
//
//  Warning: Caller must free memory allocated by this function using SMO_free_double().
//
{
	long size;
	double* array;

	if (smoapi->isOpened)
	{
		size = smoapi->Nperiods;

		array = (double*)calloc(size, sizeof(double));
		*errcode = (MEMCHECK(array));

		return array;
	}
	*errcode = 412;
	return NULL;
}

int DLLEXPORT SMO_getTimeList(SMOutputAPI* smoapi, double* array)
//
//	Purpose: Return list of all times corresponding to computed results in decimal days since 12/13/1899.
//			 Note that the initial conditions (time 0) are not included in the file. 
{
	long k;

	if (smoapi->isOpened)
	{
		if (array == NULL) return 411;

		// loop over and build time series
		for (k = 0; k < smoapi->Nperiods; k++)
			array[k] = getTimeValue(smoapi, k);

		return 0;
	}
	
	// Error no results to report on binary file not opened
	return 412;
}



int DLLEXPORT SMO_getSubcatchSeries(SMOutputAPI* smoapi, int subcatchIndex,
	SMO_subcatchAttribute attr, long timeIndex, long length, float* outValueSeries)
//
//  Purpose: Get time series results for particular attribute. Specify series
//  start and length using timeIndex and length respectively.
//
{
	long k;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueSeries == NULL) return 411;

		// loop over and build time series
		for (k = 0; k < length; k++)
			outValueSeries[k] = getSubcatchValue(smoapi, timeIndex + k,
			subcatchIndex, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}


int DLLEXPORT SMO_getNodeSeries(SMOutputAPI* smoapi, int nodeIndex, SMO_nodeAttribute attr,
	long timeIndex, long length, float* outValueSeries)
//
//  Purpose: Get time series results for particular attribute. Specify series
//  start and length using timeIndex and length respectively.
//
{
	long k;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueSeries == NULL) return 411;

		// loop over and build time series
		for (k = 0; k < length; k++)
			outValueSeries[k] = getNodeValue(smoapi, timeIndex + k,
			nodeIndex, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}


int DLLEXPORT SMO_getLinkSeries(SMOutputAPI* smoapi, int linkIndex, SMO_linkAttribute attr,
	long timeIndex, long length, float* outValueSeries)
//
//  Purpose: Get time series results for particular attribute. Specify series
//  start and length using timeIndex and length respectively.
//
{
	long k;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueSeries == NULL) return 411;

		// loop over and build time series
		for (k = 0; k < length; k++)
			outValueSeries[k] = getLinkValue(smoapi, timeIndex + k, linkIndex, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}



int DLLEXPORT SMO_getSystemSeries(SMOutputAPI* smoapi, SMO_systemAttribute attr,
	long timeIndex, long length, float *outValueSeries)
//
//  Purpose: Get time series results for particular attribute. Specify series
//  start and length using timeIndex and length respectively.
//
{
	long k;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueSeries == NULL) return 411;

		// loop over and build time series
		for (k = 0; k < length; k++)
			outValueSeries[k] = getSystemValue(smoapi, timeIndex + k, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}

int DLLEXPORT SMO_getSubcatchAttribute(SMOutputAPI* smoapi, long timeIndex,
	SMO_subcatchAttribute attr, float* outValueArray)
//
//   Purpose: For all subcatchments at given time, get a particular attribute.
//
{
	long k;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// loop over and pull result
		for (k = 0; k < smoapi->Nsubcatch; k++)
			outValueArray[k] = getSubcatchValue(smoapi, timeIndex, k, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;

}



int DLLEXPORT SMO_getNodeAttribute(SMOutputAPI* smoapi, long timeIndex,
	SMO_nodeAttribute attr, float* outValueArray)
//
//  Purpose: For all nodes at given time, get a particular attribute.
//
{
	long k;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// loop over and pull result
		for (k = 0; k < smoapi->Nnodes; k++)
			outValueArray[k] = getNodeValue(smoapi, timeIndex, k, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;

}

int DLLEXPORT SMO_getLinkAttribute(SMOutputAPI* smoapi, long timeIndex,
	SMO_linkAttribute attr, float* outValueArray)
//
//  Purpose: For all links at given time, get a particular attribute.
//
{
	long k;

	if (smoapi->isOpened)
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// loop over and pull result
		for (k = 0; k < smoapi->Nlinks; k++)
			outValueArray[k] = getLinkValue(smoapi, timeIndex, k, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;

}


int DLLEXPORT SMO_getSystemAttribute(SMOutputAPI* smoapi, long timeIndex,
	SMO_systemAttribute attr, float* outValueArray)
//
//  Purpose: For the system at given time, get a particular attribute.
//
{
	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// don't need to loop since there's only one system
		outValueArray[0] = getSystemValue(smoapi, timeIndex, attr);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;

}

int DLLEXPORT SMO_getSubcatchResult(SMOutputAPI* smoapi, long timeIndex, int subcatchIndex,
	float* outValueArray)
//
// Purpose: For a subcatchment at given time, get all attributes.
// 
{
	ULLCAST offset;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// --- compute offset into output file
		offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)(timeIndex)*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
		// add offset for subcatchment
		offset += ((ULLCAST)subcatchIndex*(ULLCAST)smoapi->SubcatchVars)*(ULLCAST)RECORDSIZE;

		fseeko(smoapi->file, offset, SEEK_SET);
		fread(outValueArray, RECORDSIZE, smoapi->SubcatchVars, smoapi->file);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}


int DLLEXPORT SMO_getNodeResult(SMOutputAPI* smoapi, long timeIndex, int nodeIndex,
	float* outValueArray)
//
//	Purpose: For a node at given time, get all attributes.
//
{
	ULLCAST offset;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// calculate byte offset to start time for series
		offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)(timeIndex)*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
		// add offset for subcatchment and node
		offset += ((ULLCAST)smoapi->Nsubcatch*(ULLCAST)smoapi->SubcatchVars + (ULLCAST)nodeIndex*(ULLCAST)smoapi->NodeVars)*(ULLCAST)RECORDSIZE;

		fseeko(smoapi->file, offset, SEEK_SET);
		fread(outValueArray, RECORDSIZE, smoapi->NodeVars, smoapi->file);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}


int DLLEXPORT SMO_getLinkResult(SMOutputAPI* smoapi, long timeIndex, int linkIndex,
	float* outValueArray)
//
//	Purpose: For a link at given time, get all attributes.
//
{
	ULLCAST offset;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// calculate byte offset to start time for series
		offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)(timeIndex)*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
		// add offset for subcatchment and node and link
		offset += ((ULLCAST)smoapi->Nsubcatch*(ULLCAST)smoapi->SubcatchVars
			+ (ULLCAST)smoapi->Nnodes*(ULLCAST)smoapi->NodeVars + (ULLCAST)linkIndex*(ULLCAST)smoapi->LinkVars)*(ULLCAST)RECORDSIZE;

		fseeko(smoapi->file, offset, SEEK_SET);
		fread(outValueArray, RECORDSIZE, smoapi->LinkVars, smoapi->file);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}

int DLLEXPORT SMO_getSystemResult(SMOutputAPI* smoapi, long timeIndex, float* outValueArray)
//
//	Purpose: For the system at given time, get all attributes.
//
{
	ULLCAST offset;

	if (smoapi->isOpened) 
	{
		// Check memory for outValues
		if (outValueArray == NULL) return 411;

		// calculate byte offset to start time for series
		offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)(timeIndex)*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
		// add offset for subcatchment and node and link (system starts after the last link)
		offset += ((ULLCAST)smoapi->Nsubcatch*(ULLCAST)smoapi->SubcatchVars + (ULLCAST)smoapi->Nnodes*(ULLCAST)smoapi->NodeVars
			+ (ULLCAST)smoapi->Nlinks*(ULLCAST)smoapi->LinkVars)*(ULLCAST)RECORDSIZE;

		fseeko(smoapi->file, offset, SEEK_SET);
		fread(outValueArray, RECORDSIZE, smoapi->SysVars, smoapi->file);

		return 0;
	}
	// Error no results to report on binary file not opened
	return 412;
}

void DLLEXPORT SMO_free(float *array)
//
//  Purpose: frees memory allocated using SMO_newOutValueSeries() or
//  SMO_newOutValueArray().
//
{
	if (array != NULL)
		free(array);
}

void DLLEXPORT SMO_freeIDList(struct IDentry* head)
{
	struct IDentry* temp;

	while (head != NULL)
	{
		temp = head;
		head = head->nextID;
		free(temp->IDname);
		free(temp);
		temp = NULL;
	}
}

void DLLEXPORT SMO_freeTimeList(double *array)
//
//  Purpose: frees memory allocated using SMO_newTimeList
//
{
	if (array != NULL)
		free(array);
}

int DLLEXPORT SMO_close(SMOutputAPI* smoapi)
//
//   Purpose: Clean up after and close Output API
//
{
	if (smoapi->isOpened) 
	{
		fclose(smoapi->file);
		smoapi->isOpened = false;
		free(smoapi);
		smoapi = NULL;
	}
	// Error binary file not opened
	else return 412;

	return 0;
}

int DLLEXPORT SMO_errMessage(int errcode, char* errmsg, int n)
//
//  Purpose: takes error code returns error message
//
//  Input Error 411: no memory allocated for results
//  Input Error 412: no results binary file hasn't been opened
//  Input Error 421: invalid parameter code
//  File Error  434: unable to open binary output file
//  File Error  435: run terminated no results in binary file
{
	switch (errcode)
	{
	case 411: strncpy(errmsg, ERR411, n); break;
	case 412: strncpy(errmsg, ERR412, n); break;
	case 421: strncpy(errmsg, ERR421, n); break;
	case 434: strncpy(errmsg, ERR434, n); break;
	case 435: strncpy(errmsg, ERR435, n); break;
	default: return 421;
	}

	return 0;
}


// Local functions:
double DLLEXPORT getTimeValue(SMOutputAPI* smoapi, long timeIndex)
{
	ULLCAST offset;
	double value;

	// --- compute offset into output file
	offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)timeIndex*(ULLCAST)smoapi->BytesPerPeriod;

	// --- re-position the file and read the result
	fseeko(smoapi->file, offset, SEEK_SET);
	fread(&value, RECORDSIZE * 2, 1, smoapi->file);

	return value;
}


float DLLEXPORT getSubcatchValue(SMOutputAPI* smoapi, long timeIndex, int subcatchIndex,
	SMO_subcatchAttribute attr)
{
	ULLCAST offset;
	float value;

	// --- compute offset into output file
	offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)timeIndex*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
	// offset for subcatch
	offset += (ULLCAST)RECORDSIZE*((ULLCAST)subcatchIndex*(ULLCAST)smoapi->SubcatchVars + (ULLCAST)attr);

	// --- re-position the file and read the result
	fseeko(smoapi->file, offset, SEEK_SET);
	fread(&value, RECORDSIZE, 1, smoapi->file);

	return value;
}

float DLLEXPORT getNodeValue(SMOutputAPI* smoapi, long timeIndex, int nodeIndex,
	SMO_nodeAttribute attr)
{
	ULLCAST offset;
	float value;

	// --- compute offset into output file
	offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)timeIndex*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
	// offset for node
	offset += (ULLCAST)RECORDSIZE*((ULLCAST)smoapi->Nsubcatch*(ULLCAST)smoapi->SubcatchVars + (ULLCAST)nodeIndex*(ULLCAST)smoapi->NodeVars + (ULLCAST)attr);

	// --- re-position the file and read the result
	fseeko(smoapi->file, offset, SEEK_SET);
	fread(&value, RECORDSIZE, 1, smoapi->file);

	return value;
}


float DLLEXPORT getLinkValue(SMOutputAPI* smoapi, long timeIndex, int linkIndex,
	SMO_linkAttribute attr)
{
	ULLCAST offset;
	float value;
	
	// --- compute offset into output file
	offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)timeIndex*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
	// offset for link
	offset += (ULLCAST)RECORDSIZE*((ULLCAST)smoapi->Nsubcatch*(ULLCAST)smoapi->SubcatchVars + (ULLCAST)smoapi->Nnodes*(ULLCAST)smoapi->NodeVars +
		(ULLCAST)linkIndex*(ULLCAST)smoapi->LinkVars + (ULLCAST)attr);
		
	// --- re-position the file and read the result
	fseeko(smoapi->file, offset, SEEK_SET);
	fread(&value, RECORDSIZE, 1, smoapi->file);

	return value;
}

float DLLEXPORT getSystemValue(SMOutputAPI* smoapi, long timeIndex,
	SMO_systemAttribute attr)
{
	ULLCAST offset;
	float value;

	// --- compute offset into output file
	offset = (ULLCAST)smoapi->ResultsPos + (ULLCAST)timeIndex*(ULLCAST)smoapi->BytesPerPeriod + (ULLCAST)2 * (ULLCAST)RECORDSIZE;
	//  offset for system
	offset += (ULLCAST)RECORDSIZE*((ULLCAST)smoapi->Nsubcatch*(ULLCAST)smoapi->SubcatchVars + (ULLCAST)smoapi->Nnodes*(ULLCAST)smoapi->NodeVars +
		(ULLCAST)smoapi->Nlinks*(ULLCAST)smoapi->LinkVars + (ULLCAST)attr);

	// --- re-position the file and read the result
	fseeko(smoapi->file, offset, SEEK_SET);
	fread(&value, RECORDSIZE, 1, smoapi->file);

	return value;
}