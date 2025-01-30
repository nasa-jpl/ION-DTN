/*

	ionexit.c:	Cleanly shut down ION.


        Written 5/2011 by Greg Menke, Columbus, under contract with NASA GSFC

*/



#include "bp.h"
#include "bpP.h"

#include "bssp.h"
#include "bssp/library/bsspP.h"

#include "cfdp.h"
#include "cfdp/library/cfdpP.h"

#include "ltp.h"
#include "ltpP.h"

#include "rfx.h"



static void	printText(char *text)
{
   PUTS(text);
}



#if defined (ION_LWT)
int	ionexit(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
   char	*p1 = (char *) a1;
#else
int	main(int argc, char **argv)
{
   char	*p1 = (argc > 1 ? argv[1] : NULL);
#endif
   int loopcount, errcount= 0, deletesdr = -1;


   if( p1 != NULL )
   {
      if( strcmp( p1, "k" ) == 0 )
      {
         deletesdr = 0;
      }
   }


   printText("Running ionexit" );
   printText( ((deletesdr) ? "will delete SDR" : "keeping SDR") );

   if (ionAttach() == 0)
   {
      if (bpAttach() == 0)
      {
         printText("Issuing BP stop.");
         bpStop();

         for( loopcount= 5; bp_agent_is_started() && loopcount; loopcount--)
         {
            snooze(1);
         }
         if( !loopcount )
         {
            errcount++;
            printText("***** BP did not shut down");
         }
      }
      else
         printText("Unable to attach to BP");



      if (ltpAttach() == 0)
      {
         printText("Issuing LTP stop.");
         ltpStop();

         for( loopcount = 5; ltp_engine_is_started() && loopcount; loopcount-- )
         {
            snooze(1);
         }
         if( !loopcount )
         {
            errcount++;
            printText("***** LTP did not shut down");
         }
      }
      else
         printText("Unable to attach to LTP");




      if (bsspAttach() == 0)
      {
         printText("Issuing BSSP stop.");
         bsspStop();

         for( loopcount = 5; bssp_engine_is_started() && loopcount; loopcount-- )
         {
            snooze(1);
         }
         if( !loopcount )
         {
            errcount++;
            printText("***** BSSP did not shut down");
         }
      }
      else
         printText("Unable to attach to BSSP");




      if (cfdpAttach() == 0)
      {
         printText("Issuing CFDP stop.");
         cfdpStop();

         for( loopcount = 5; cfdp_entity_is_started() && loopcount; loopcount-- )
         {
            snooze(1);
         }
         if( !loopcount )
         {
            errcount++;
            printText("***** CFDP did not shut down");
         }
      }
      else
         printText("Unable to attach to CFDP");




      {
         printText("Issuing RFX stop.");
         rfx_stop();

         for( loopcount= 5; rfx_system_is_started() && loopcount; loopcount-- )
         {
            snooze(1);
         }
         if( !loopcount )
         {
            errcount++;
            printText("***** RFX did not shut down");
         }
      }


      {
         if( deletesdr )
         {
            printText("Deleting SDR");
            ionTerminate();
         }

         printText("Shutting down SDR");

         sdr_shutdown();

         printText("Calling sm_ipc_stop()");

         sm_ipc_stop();
      }


      //ionDetach();
   }
   else
      printText("Unable to attach to ION");



   printText("Stopping ionexit.");

   return (errcount != 0)? -1 : 0;
}


/* eof */
