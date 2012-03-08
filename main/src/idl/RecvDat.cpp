#include <MRIData.h>
#include <TCPCommunicator.h>
//#include <Serializable.h>
#include <MRIDataComm.h>
#include <SiemensTool.h>
#include <idl/IDLData.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <idl_export.h> 
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

IDL_VPTR RecvDat (int, IDL_VPTR *);
int IDL_Load (void);

#ifdef __cplusplus
} 
#endif

/* Define message codes */
static IDL_MSG_DEF msg_arr[] = { 
#define M_RD_LISTEN_FAIL               0   
  {  "M_RD_LISTEN_FAIL",   "%Ncommunicator.Listen() failed!" }, 
#define M_RD_RECEIVERR_FAIL            -1   
  {  "M_RD_RECEIVERR_FAIL",   "%Ncommunicator.ReceiveReconRequest() failed!" }, 
#define M_RD_RECEIVED_FAIL             -2   
  {  "M_RD_RECEIVED_FAIL",   "%Ncommunicator.ReceiveData() failed!" }, 
#define M_RD_ACCEPTCON_FAIL             -2   
  {  "M_RD_ACCEPTCON_FAIL",   "%Ncommunicator.AcceptConnection() failed!" }, 
}; 

/* The load function fills in this message block handle with the 
 * opaque handle to the message block used for this module. The other 
 * routines can then use it to throw errors from this block. */ 
static IDL_MSG_BLOCK msg_block; 


IDL_VPTR RecvDat (int argc, IDL_VPTR *argv) {

   IDL_VPTR idl_Port;
   IDL_VPTR idl_Data;

   idl_Port = argv[0];

   IDL_ENSURE_SIMPLE(idl_Port);
   IDL_ENSURE_SCALAR(idl_Port);
   IDL_EXCLUDE_COMPLEX(idl_Port);
   IDL_EXCLUDE_UNDEF(idl_Port);

   // Convert to int
   if (idl_Port->type != IDL_TYP_LONG)
      idl_Port = IDL_CvtLng(1, argv);

   int port = IDL_LongScalar(idl_Port);

   // connect
   TCPCommunicator communicator;
   if( !communicator.Listen( port ) )
      IDL_MessageFromBlock(msg_block, M_RD_LISTEN_FAIL, IDL_MSG_RET);

   IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "waiting for connection...");

   if( communicator.AcceptConnection() )
   {
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "client connected...");

      // get recon request
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "   getting request...");
      MRIReconRequest request;
      if( !communicator.ReceiveReconRequest( request ) )
         IDL_MessageFromBlock(msg_block, M_RD_RECEIVERR_FAIL, IDL_MSG_RET);
         
      // get data
      MRIData mri_data;
      if( !communicator.ReceiveData( mri_data ) )
         IDL_MessageFromBlock(msg_block, M_RD_RECEIVED_FAIL, IDL_MSG_RET);

      idl_Data = IDLDataExportIDLArray( mri_data );

   }
   else
   {
      IDL_MessageFromBlock(msg_block, M_RD_ACCEPTCON_FAIL, IDL_MSG_RET);
   }

   // close connection
   communicator.CloseConnection();
   IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "Done!");

   return idl_Data;

}

int IDL_Load(void)
{
   static IDL_SYSFUN_DEF2 function_addr[] = {
      { (IDL_SYSRTN_GENERIC) RecvDat, "RECVDAT", 1, 1, 0, 0 },
   };

   /* Create a message block to hold our messages. Save its handle where 
    * the other routines can access it. */ 
  if (!(msg_block = IDL_MessageDefineBlock("IDLIceClient", 
                                           IDL_CARRAY_ELTS(msg_arr), 
                                           msg_arr))) return IDL_FALSE; 
  
  /* Register our routine. The routines must be specified exactly the same 
   * as in testmodule.dlm. */ 
  return IDL_SysRtnAdd(function_addr, TRUE,  
                       IDL_CARRAY_ELTS(function_addr));
}
