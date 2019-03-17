#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "xil_io.h"
#include "xsdps.h"		/* SD device driver */
#include "xil_cache.h"
#include "xplatform_info.h"
#include "ff.h"
#include "math.h"
#include "functions.h"
#include "det_parameters.h"


#define MAX_PKT_LEN (N_bands*N_pixels)
#define NUMBER_OF_TRANSFERS (1)



/*****************************************************************************/

int main()
{


	init_platform();

	//TIMER
	XTime tStart, tEnd;

    //Start timer
    XTime_GetTime(&tStart);

    //HW timer
    init_timer ( XPAR_AXI_TIMER_0_DEVICE_ID , 0 );
    u32 value1 = start_timer ( 0 );

    //IMPORT HYPERSPECTRAL CUBE TO DDR
    read_data(HyperMatrixFile,HyperData);

    //IMPORT HYPERSPECTRAL Target TO DDR
    read_data(TargetFile,target);

#ifdef DEBUG
    printf("%d %d %d \n",HyperData[0],HyperData[1],HyperData[2]);

    //Calculate Correlation Matrix R
    printf("Started calculating R...\n");
#endif

    hyperCorr (HyperData, R,  N_bands, N_pixels,100);

#ifdef DEBUG
    printf("%f %f %f \n", R[0][0],R[0][1],R[0][2]);
#endif

    //Calculate LU decomposition
    if(LUPdecompose(N_bands, R, P) < 0)
    	{
    		printf("The LUP decomposition of 'R' unsuccessful.\n");
    		return -1;
    	}


    //Calculate LU inversion
    if(LUPinverse(N_bands, P, R,B,X,Y) < 0)
		{
			printf("Matrix inversion unsuccessful.\n");
			return -1;
		}

#ifdef DEBUG
    printf("inverted: \n");
    printf("%.15f %.15f %.15f \n", R[0][0],R[0][1],R[0][2]);
#endif

	//find maximum in R
	double maximum = R[0][0];
	double minimum = R[0][0];

	 for(int i=0;i<N_bands; i++){
    	for(int j=0;j<N_bands;j++){
    		if ( R[i][j] > maximum )
				maximum = R[i][j];
			if ( R[i][j] < minimum )
				minimum = R[i][j];	
    	}
    }	
	
	double p1 = fabs(floor((1<<30)/maximum));
    double p2 = fabs(floor((1<<30)/minimum));


     if(p1 > p2)
        p1 = p2;

	p1 = floor(log2(p1));
   
    double shift = pow(2,p1);

	//determine normalization and convert to 2^30 range
	for(int i=0;i<N_bands; i++){
    	for(int j=0;j<N_bands;j++){
    		R[i][j] *= shift;
        }
	}
		
    //convert to FIXED POINT
    for(int i=0;i<N_bands; i++){
    	for(int j=0;j<N_bands;j++){
    		R32[i][j]= (s32)( R[i][j]);
    	}
    }

#ifdef DEBUG
	printf("shifted by 2 ^ %f \n",p1);
    printf("inverted fixed point \n");
    printf("%ld %ld %ld \n", R32[0][0],R32[0][1],R32[0][2]);
    printf("%ld %ld %ld \n", R32[1][0],R32[1][1],R32[1][2]);
    printf("%ld %ld %ld \n", R32[2][0],R32[2][1],R32[2][2]);
    printf("%ld %ld %ld \n", R32[15][0],R32[15][1],R32[15][2]);
#endif

    //Prepare s'*R^-1
    arrayMatrixProduct(N_bands,target,R,sR);

	//find maximum in sR
	maximum = sR[0];
	minimum = sR[0];

	 for(int i=0;i<N_bands; i++){
		if ( sR[i] > maximum )
			maximum = sR[i];
		if ( sR[i] < minimum )
			minimum = sR[i];	
    }
	
	p1 = fabs(floor((1<<30)/maximum));
	p2 = fabs(floor((1<<30)/minimum));
	
	 if(p1 > p2)
        p1 = p2;

	p1 = floor(log2(p1));
   
	shift = pow(2,p1);
	
	for(int i = 0;i < N_bands; i++){
		sR[i] *= shift;
    	}

    //convert to FIXED POINT MULT WITH 2^36
    for(int j = 0;j < N_bands; j++){
        		sR32[j]	= (s32)( sR[j]);
        	}

#ifdef DEBUG
	printf("shifted by 2 ^ %f \n",p1);
    printf("sR fp: \n");
    printf("%ld %ld %ld \n", sR32[0],sR32[1],sR32[2]);
#endif

    //Prepare sR^-1*s
    double sRs = scalarProduct(sR,target,N_bands);
    //double temp1,temp2;
	
	p1 = fabs(floor((1<<30)/sRs));
	p1 = floor(log2(p1));   
	shift = pow(2,p1);
	
	s32 sRs32 = (s32)(sRs*shift);

#ifdef DEBUG
	printf("shifted by 2 ^ %f \n",p1);
    printf("sRs is %f \n", sRs);
    printf("Start calculating ACE...\n");
#endif


    //HW ACCELERATOR
#ifdef DEBUG
    xil_printf("UPLOAD MATRIX \n");
#endif

	//disable debug while writing
	*(BRAM_BASE_ADDR + 3) = 0;

    //WRITING inverted correlation matrix to BRAM
    for(int i = 0; i < N_bands; i++){
       	for(int j = 0; j< N_bands; j++){

       		*(BRAM_BASE_ADDR) = R32[i][j];
       	}
       }

#ifdef DEBUG
    xil_printf("UPLOADED! \n");

    xil_printf("UPLOAD ARRAY SR \n");
#endif


    //WRITING sR^-1 vector to BRAM
	for (int i = 0; i< N_bands; i++){

			*(BRAM_BASE_ADDR + 1) = sR32[i];
		}
		
	//WRITING sRs 
	*(BRAM_BASE_ADDR + 2) = sRs32;


#ifdef DEBUG
	/*xil_printf("UPLOADED! \n");

    	xil_printf("DEBUG  SR \n");

    	//selection initial 0
    	*(BRAM_BASE_ADDR + 2) = 0;

    	    	for (int i = 0; i< N_bands; i++)
    	    			{
    	    		*(BRAM_BASE_ADDR + 2) = i;
    	    		printf("sr3: %d; ", *(BRAM_BASE_ADDR + 3));
    	    		printf("sr2: %d; ", *(BRAM_BASE_ADDR + 2));
    	    		printf("MAT: %d; ", *(BRAM_BASE_ADDR));
    	    		printf("SR: %d; \n", *(BRAM_BASE_ADDR + 1));
    	    			}
    	    	xil_printf("END DEBUG \n");

    	//disable debug
    	*(BRAM_BASE_ADDR + 3) = 0;*/
#endif




	int Status;

	Status = setup_DMA();
	if (Status != XST_SUCCESS) {
			xil_printf("FAILED SETTING UP DMA \n");
					return XST_FAILURE;
				}


	//send packets
	Status = main_DMA(HyperData, receiver, MAX_PKT_LEN, NUMBER_OF_TRANSFERS);
	if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}

	ReceiveData (receiver,N_pixels);

#ifdef DEBUG
    	for(int i = 0 ; i<40; i++)
    		xil_printf("%d; ", receiver[i]);
#endif


    //last multiplication
    	/*for(int i=0; i < N_pixels; i++)
    		result[i] = (double)receiver[i] * sRs;*/

#ifdef DEBUG
    	/*for(int i=0; i < 40; i++)
    		printf("%f %f; ", (double)receiver[i],result[i]);*/
#endif

    //HW ACCELERATOR END
    write_data(ResultsFile, receiver);

    //Stop timer
    XTime_GetTime(&tEnd);

    //HW timer
    u32 value2 = stop_timer ( 0 );

    xil_printf ("\n Timer : %d\n", value2 - value1 );

  printf("t=%15.5lf sec\n",(long double)((tEnd-tStart) *2)/(long double)XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ);
  printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
  printf("Output took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));

  cleanup_platform();

  return 0;

}
