/*
 eclsaCodecAdapter_LibecDLR.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#include "eclsaCodecAdapter.h"
#include <libec.h>  //libec decoder
#include <unistd.h>
#include <fcntl.h>  //open flag O_RDONLY
#include <dirent.h> //opendir , readdir , rewinddir , closedir
#include <string.h> //strtok
#include <stdlib.h> //exit

#define FEC_DEFAULT_ALPHAMAX 1000
#define FEC_BUF_SIZE 256
#define FEC_MATRIX_PATH "/tmp/matrix/"

typedef enum
{
	STATUS_LIBEC_SUCCESS_ML = 2,
	STATUS_LIBEC_SUCCESS_IT = 1,
	STATUS_LIBEC_FAILED_IT = 0,
	STATUS_LIBEC_FAILED_ML = -1,
	STATUS_LIBEC_FAILED_ALPHA_TOO_BIG = -2
} LibecCodecStatus;

typedef struct
{
	int		maxCNDegreeHc;
	node 	*CN;
	node 	*VN;
	int 	**Hc;
	int 	*CNDegreeHc;
	uint8b 	**C;
} LibecVars; //vars to libec only (universal_encoder)

/*Auxiliary functions for FECArrayLoad() */
static int readWord(int fd,char *buffer, int maxLineSize)
{
	//Read one word from the file, i.e. a set of consecutive characters
	//separated by a space, a tab or a newline.
	int i=0;
	int byteRead;
	char readChar;
	while((byteRead=read(fd,&readChar,1)) == 1 && i < maxLineSize-1 && readChar!=' ' && readChar!='\t' && readChar!='\n')
		  buffer[i++]=readChar;
	buffer[i]='\0';
	return byteRead;
}
static int readLine(int fd,char *buffer, int maxLineSize)
{
	//Read one line from the file, i.e. a set of consecutive characters separated by newline
	int i=0;
	int byteRead;
	char readChar;
	while((byteRead=read(fd,&readChar,1)) == 1 && i < maxLineSize-1 && readChar!='\n')
		  buffer[i++]=readChar;
	buffer[i]='\0';
	return byteRead;
}
static int jumpToChar(int fd,char wchar)
{
	//Read until the wanted char (wchar) is found
	int byteRead;
	char readChar;
	while((byteRead=read(fd,&readChar,1)) == 1 && readChar!=wchar);
	return byteRead;
}
static int getMaxCNDegreeHc(FecElement *fecCode,char *filename)
{
	//Determine the max CN degree from matrix file
	int fd;
	char buffer[FEC_BUF_SIZE];
	int i;
	int CNDegree;
	int tmpM= fecCode->N - fecCode->K;
	int maxCNDegree=0;

	fd = open(filename,O_RDONLY);

	//Read Hc
	for(i=0;i<tmpM;i++)
		{
		jumpToChar(fd,'>');
		jumpToChar(fd,',');
		readWord(fd,buffer,FEC_BUF_SIZE);
		CNDegree=atoi(buffer);

		if(CNDegree>maxCNDegree)
			maxCNDegree=CNDegree;
		}
	close(fd);
	return maxCNDegree;
}
static int getValueArrayIndex(int *array, int arrayLength, int value)
{
   //Return the (first) array index of a value contained in the array.
   int i;
   for (i=0;i<arrayLength;i++)
      if (array[i] == value)
	    return i;

   return -1;  //not found
}
static void getCodeParametersFromFile(char *filepath,int *K, int *N,int *ID)
{
	  char buffer[FEC_BUF_SIZE];
	  int fd;
	  char *token;
	  fd = open(filepath,O_RDONLY);
	  //TODO file must be validated before parsing!
	  //parse file
	  *K=-1;
	  *N=-1;
	  *ID=0;
	  while (readLine(fd,buffer, FEC_BUF_SIZE) && (*K == -1 || *N == -1 || *ID == 0))
	  {
		  //todo the following code works but it may be improved
		  if (strstr(buffer, "K =") != NULL)
			 {
			  token=strtok(buffer, "=");
			  token=strtok(NULL, "=");
			  *K=atoi(token);
			 }
		  if (strstr(buffer, "N =") != NULL)
			 {
			  token=strtok(buffer, "=");
			  token=strtok(NULL, "=");
			  *N=atoi(token);
			 }
		  if (strstr(buffer, "ID =") != NULL)
			 {
			  token=strtok(buffer, "=");
			  token=strtok(NULL, "=");
			  *ID=atoi(token);
			 }
	  }
	  close(fd);
}
static void loadLibecVars(FecElement *tmpArray,int arrayLength, char ** pathlist)
{
	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	// LOADING LIBECVARS
	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////

		int fecIndex;
		int fecM;
		int *VNDegreeHc;
		int maxVNDegreeHc;
		int **Hv;
		int *VNDegreeTemp;
		int column;
		LibecVars *libecVars;
		FecElement *fec;
		char 			buffer[FEC_BUF_SIZE];
		int i,j,fd;

		for(fecIndex=0; fecIndex < arrayLength ; fecIndex++)
		{
			fec=&(tmpArray[fecIndex]);
			libecVars= fec->codecVars;
			fecM = fec->N - fec->K;
			libecVars->maxCNDegreeHc=getMaxCNDegreeHc(fec,pathlist[fecIndex]);

			//allocate memory
			libecVars->Hc=(int **)malloc(sizeof(int *)*fecM);
			for(j=0;j<fecM;j++)
				libecVars->Hc[j]=(int *)malloc(sizeof(int)* libecVars->maxCNDegreeHc);

			libecVars->CNDegreeHc=(int *)malloc(sizeof(int)*fecM);
			VNDegreeHc=(int *)malloc(sizeof(int)*fec->N);

			//for the ML decoder
			libecVars->C=(uint8b **)malloc(sizeof(uint8b *)*fecM);
			for(i=0;i<fecM;i++)
				libecVars->C[i]=(uint8b *)malloc(sizeof(uint8b)*fec->T);			//->check memory allocation

			//read parity check matrix and vn degrees
			memset(VNDegreeHc,0,sizeof(int) *fec->N );
			//fd = open(fec->fileName,O_RDONLY);
			fd = open(pathlist[fecIndex],O_RDONLY);

			//read matrix file
			for(i=0;i< fecM ;i++)
				{
				jumpToChar(fd,'>');
				jumpToChar(fd,',');
				readWord(fd,buffer, FEC_BUF_SIZE);
				libecVars->CNDegreeHc[i]=atoi(buffer);

				if(libecVars->CNDegreeHc[i]>libecVars->maxCNDegreeHc)
					{
					printf("\n ERROR: Hc needs at least %d columns",libecVars->CNDegreeHc[i]);
					exit(0);
					}

				for(j=0;j<libecVars->CNDegreeHc[i];j++)
					{
					jumpToChar(fd,':');
					readWord(fd,buffer, FEC_BUF_SIZE);
					libecVars->Hc[i][j]=atoi(buffer);
					VNDegreeHc[libecVars->Hc[i][j]]++;
					}
			}
			close(fd);

			//determine max VN degree
			maxVNDegreeHc=0;
			for (i=0;i<fec->N;i++)
				if (VNDegreeHc[i]>maxVNDegreeHc)
					maxVNDegreeHc=VNDegreeHc[i];

			//allocate memory
			Hv=(int **)malloc(sizeof(int *)*fec->N);
			for(j=0;j<fec->N;j++)
				Hv[j]=(int *)malloc(sizeof(int)* maxVNDegreeHc);

			//generate Hv
			VNDegreeTemp=(int *)malloc(sizeof(int)*fec->N);
			memset(VNDegreeTemp,0, sizeof(int) * fec->N);

			for(i=0;i<fecM;i++)
			{
				for(j=0;j<libecVars->CNDegreeHc[i];j++)
				{
				column=libecVars->Hc[i][j];
				Hv[column][VNDegreeTemp[column]]=i;
				VNDegreeTemp[column]++;
				}
			}

			//allocate memory
			libecVars->CN=(node *)malloc(sizeof(node)*fecM);

			for(j=0;j<fecM;j++)
				{
				libecVars->CN[j].conNodes=	(int *)malloc(sizeof(int)		*(libecVars->CNDegreeHc[j]+FEC_DEFAULT_ALPHAMAX));
				libecVars->CN[j].conStatus=	(uint8 *)malloc(sizeof(uint8)	*(libecVars->CNDegreeHc[j]+FEC_DEFAULT_ALPHAMAX));
				libecVars->CN[j].posXList=	(int *)malloc(sizeof(int)		*(libecVars->CNDegreeHc[j]+FEC_DEFAULT_ALPHAMAX));
				}

			//allocate memory
			libecVars->VN=(node *)malloc(sizeof(node)*fec->N);

			for(j=0;j<fec->N;j++)
				{
				libecVars->VN[j].conNodes=(int *)malloc(sizeof(int)		*VNDegreeHc[j]);
				libecVars->VN[j].conStatus=(uint8 *)malloc(sizeof(uint8)*VNDegreeHc[j]);
				libecVars->VN[j].posXList=(int *)malloc(sizeof(int)		*VNDegreeHc[j]);
				}

			//initialize the graph (p1)
			for(i=0;i<fec->N;i++)
				{
					libecVars->VN[i].ODEGREE=VNDegreeHc[i];
					for(j=0;j<VNDegreeHc[i];j++)
					{
						libecVars->VN[i].conNodes[j]=Hv[i][j];
					}
				}

			for(i=0;i<fecM;i++)
			{
				libecVars->CN[i].ODEGREE=libecVars->CNDegreeHc[i];

				for(j=0;j<libecVars->CN[i].ODEGREE;j++)
				{
					libecVars->CN[i].conNodes[j]=libecVars->Hc[i][j];
					libecVars->CN[i].posXList[j]=getValueArrayIndex(libecVars->VN[libecVars->CN[i].conNodes[j]].conNodes,VNDegreeHc[libecVars->CN[i].conNodes[j]],i);
					libecVars->VN[libecVars->CN[i].conNodes[j]].posXList[libecVars->CN[i].posXList[j]]=j;
				}

			}

			//release the auxiliary memory
			free(VNDegreeTemp);
			for(j=0;j<fec->N;j++)
				free(Hv[j]);
			free(Hv);
			free(VNDegreeHc);

			free(pathlist[fecIndex]);
			debugPrint("FEC:loaded matrix K %d N %d", fec->K, fec->N);
		}

}

/*Fec Array*/
void FECArrayLoad(int envT)
{
	//Load the FEC Array from file.
	  DIR           *directory;
	  struct dirent *dir;
	  int 			fd;
	  char 			fullPath[PATH_MAX+NAME_MAX];
	  char			*pathlist[FEC_CODE_MAX_COUNT];
	  char 			buffer[FEC_BUF_SIZE];
	  int 			currentK,currentN,currentID;
	  int 			i,j;
	  FecElement 	*fec;
	  FecElement	tmpArray[FEC_CODE_MAX_COUNT];

	  int arrayLength=0;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// COMPUTE THE FEC ARRAY SIZE
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

	  directory = opendir(FEC_MATRIX_PATH);
	  if (directory)
		  {
		  while ((dir = readdir(directory)) != NULL) //for each file in directory...
		  	if (dir->d_type == DT_REG) //if file is a regular file
			  {
		  		strcpy(fullPath,FEC_MATRIX_PATH);
		  		strcat(fullPath,dir->d_name);
		  		getCodeParametersFromFile(fullPath,&currentK,&currentN,&currentID);

		  		if( isFecElementAddable(currentK,currentN) )
		  			{
		  			//TODO file must be validated
		  			arrayLength++;
		  			}
			  }
		  }
	  else
		  {
		  debugPrint("ERROR: FEC FOLDER NOT FOUND");
		  exit(1);
		  }

	  debugPrint("found %d files in the code folder",arrayLength);
	  createNewFecArray(arrayLength);


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// LOAD PARAMETERS INTO TMP ARRAY
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

	  for(i=0;i<arrayLength;i++)
		  {
		  fec=&(tmpArray[i]);
		  fec->codecVars=malloc(sizeof(LibecVars));
		  }

	  rewinddir(directory);
	  arrayLength=0;
	  if (directory)
		  {
			while ((dir = readdir(directory)) != NULL) //for each file in directory...
			{
			  if (dir->d_type == DT_REG) //if file is a regular file
				  {
				  strcpy(fullPath,FEC_MATRIX_PATH);
				  strcat(fullPath,dir->d_name);
				  getCodeParametersFromFile(fullPath,&currentK,&currentN,&currentID);

				  if( isFecElementAddable(currentK,currentN) )
					  {
					  fec=&(tmpArray[arrayLength]);
					  fec->K=currentK;
					  fec->N=currentN;
					  //todo should T and ALPHAMAX be inserted into files?
					  fec->T=envT;
					  fec->continuous=false;

					  pathlist[arrayLength]=(char*)malloc(strlen(fullPath)+1);
					  strcpy(pathlist[arrayLength],fullPath);
					  arrayLength++;
					  }
				  } //end of if (dir->d_type == DT_REG)
			} //end of while ((dir = readdir(directory)) != NULL)
		  } //end of if (directory)
	  else // directory=false
		  {
		  debugPrint("ERROR: FEC FOLDER NOT FOUND");
		  exit(1);
		  }
	closedir(directory);
    loadLibecVars(tmpArray,arrayLength,pathlist);

    // ADD FECs TO FECARRAY
    for(i=0;i<arrayLength;i++)
    	{
    	fec=&(tmpArray[i]);
    	addFecElementToArray(*fec);
    	}
}
void	destroyCodecVars(FecElement *fec)
{
	int fecIndex;
	int fecM;
	LibecVars 	*libecVars;
	int i;

	libecVars= (LibecVars *)fec->codecVars;
	fecM = fec->N - fec->K;
	///////////
	for(i=0;i<fec->N;i++)
		{
		free(libecVars->VN[i].conNodes);
		free(libecVars->VN[i].conStatus);
		free(libecVars->VN[i].posXList);
		}
	///////////
	free(libecVars->VN);
	///////////
	for(i=0;i<fecM;i++)
		{
		free(libecVars->CN[i].conNodes);
		free(libecVars->CN[i].conStatus);
		free(libecVars->CN[i].posXList);
		}
	///////////
	free(libecVars->CN);
	///////////
	for(i=0;i<fecM;i++)
		free(libecVars->C[i]);
	free(libecVars->C);
	///////////
	free(libecVars->CNDegreeHc);
	///////////
	for(i=0;i<fecM;i++)
		free(libecVars->Hc[i]);
	free(libecVars->Hc);

	free(libecVars);
}

/*Codec Matrix*/
int  encodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode)
{
	LibecVars *libecVars= encodingCode->codecVars;
	//Note that the universal_encoder actually calls the decoder_ML
	int M = encodingCode->N - encodingCode->K;
	uint8 *rowStatus = (uint8 *)calloc(M,sizeof(uint8));
	int status;

	status= universal_encoder(encodingCode->K,
							  encodingCode->N,
							  encodingCode->T,
							  codecMatrix->symbolStatus,
							  rowStatus,
							  codecMatrix->codewordBox,
							  libecVars->CNDegreeHc,
							  libecVars->maxCNDegreeHc,
							  libecVars->Hc,
							  libecVars->CN,
							  libecVars->VN,
							  libecVars->C,
							  FEC_DEFAULT_ALPHAMAX);
	free(rowStatus);
	return status;
}
int  decodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode,int paddingFrom,int paddingTo)
{
	LibecVars 	*libecVars	=   encodingCode->codecVars;
	int i;
	int M = encodingCode->N - encodingCode->K;
	uint8 *rowStatus = (uint8 *)calloc(M,sizeof(uint8));
	int status;
	//Set the libec "columnStatus" of padding segments to "valid" , otherwise they
	//would be considered as missing.
	//Note that libec use the term "column" to denote our matrix rows.
	for(i=paddingFrom ;i<  paddingTo ; i++)
		codecMatrix->symbolStatus[i]=1;

	status= 	decoder_ML(
						encodingCode->K,
						encodingCode->N,
						encodingCode->T,
						codecMatrix->symbolStatus,
						rowStatus,
						NULL,
						codecMatrix->codewordBox,
						libecVars->CNDegreeHc,
						libecVars->maxCNDegreeHc,
						libecVars->Hc,
						libecVars->CN,
						libecVars->VN,
						libecVars->C,
						FEC_DEFAULT_ALPHAMAX);
	free(rowStatus);
	return status;
	}

/**/
char convertToAbstractCodecStatus(char codecStatus)
{
	if(codecStatus == STATUS_CODEC_NOT_DECODED)
		return STATUS_CODEC_NOT_DECODED;

	if(codecStatus == STATUS_LIBEC_SUCCESS_ML || codecStatus == STATUS_LIBEC_SUCCESS_IT)
		return STATUS_CODEC_SUCCESS;
	else
		return STATUS_CODEC_FAILED;
}
char *getCodecStatusString(int codecStatus)
	{
	static char *not_decoded=	"LibecDLR:\"Not decoded\"";
	static char *success_ml=	"LibecDLR:\"ML ok\"";
	static char *success_it=	"LibecDLR:\"IT ok\"";
	static char *failed_it=		"LibecDLR:\"Failed IT\"";
	static char *failed_ml=		"LibecDLR:\"Failed ML\"";
	static char *failed_alpha=	"LibecDLR:\"Failed ALPHA\"";
	static char *unknown=		"LibecDLR:\"Unknown Decode Status\"";

	switch (codecStatus)
		{
			case STATUS_CODEC_NOT_DECODED:
					return not_decoded;
			case STATUS_LIBEC_SUCCESS_ML:
					return success_ml;
			case STATUS_LIBEC_SUCCESS_IT:
					return success_it;
			case STATUS_LIBEC_FAILED_IT:
					return failed_it;
			case STATUS_LIBEC_FAILED_ML:
					return failed_ml;
			case STATUS_LIBEC_FAILED_ALPHA_TOO_BIG:
					return failed_alpha;
			default:
					return unknown;
		}
	}
bool 	isContinuousModeAvailable()
{
return false;
}
