#include <stdio.h>
#include <idl_export.h>
#include <MRIData.h>
#include <idl/IDLData.h>
#include <GIRLogger.h>
#include <cstdio>
#include <cstring>

IDL_VPTR IDLDataExportIDLArray( MRIData& mri_data ) {

   IDL_VPTR        idl_Output;
   float           *f_src, *f_dst;
   int             idl_ReturnType;

   // create array
   const MRIDimensions size = mri_data.Size();
   IDL_MEMINT dims[] = { size.Column, size.Line, size.Channel, size.Set, size.Phase, size.Slice, size.Echo, size.Repetition, size.Segment, size.Partition, size.Average };

   idl_ReturnType = ( mri_data.IsComplex() )? IDL_TYP_COMPLEX : IDL_TYP_FLOAT;

   // IDL_MakeTempArray returns a pointer to the first element of the data
   //    idl_Output actually stores the more complicated structure including the data field
   f_dst = (float *) IDL_MakeTempArray( idl_ReturnType, 11, dims, IDL_ARR_INI_ZERO, &idl_Output );
   f_src = (float *) mri_data.GetDataStart();

   // Since IDL complex is interleaved, this should work
   memcpy( f_dst, f_src,  mri_data.NumElements() * sizeof( float ) );

   return idl_Output;

}
