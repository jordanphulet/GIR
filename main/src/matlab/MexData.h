#ifndef __MEX_EXPORT_H__
#define __MEX_EXPORT_H__

#include <mex.h>

class MRIData;

class MexData
{
	public:
	static bool ImportMexArray( MRIData& mri_data, const mxArray* mx_array, bool allow_resize );
	static mxArray* ExportMexArray( MRIData& mri_data );
	protected:
	private:
};

#endif
