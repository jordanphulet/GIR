#ifndef __RADIAL_GRIDDER_H__
#define __RADIAL_GRIDDER_H__

#include <MRIData.h>


class RadialGridder
{
	public:
	enum InterpKernelType { KERN_TYPE_BILINEAR };
	enum ViewOrderingType { VO_NONE, VO_JORDAN_JITTER, VO_GOLDEN_RATIO, VO_GOLDEN_RATIO_NO_PHASE };

	RadialGridder(): repetition_offset( 0 ), view_ordering( VO_NONE ), kernel_center( -1 ) {}
	~RadialGridder();

	int repetition_offset;
	ViewOrderingType view_ordering;

	bool Grid( const MRIData& radial_data, MRIData& cart_data, InterpKernelType kernel_type, int kernel_size, bool flatten );


	private:
	int kernel_center;
	MRIData kernel;

	bool Initialize( InterpKernelType interp_kernel_type, int kernel_size );
	void LoadBilinearKernel( int kernel_size );
	void Splat( MRIData& cartesian, MRIData& weights, float* radial_value, int cart_x, int cart_y, double splat_diff_x, double splat_diff_y, bool flatten );

};

#endif
