function [result] = remove_os( mri_data, params, meas_data )
	result = mri_data;

	% is k-space
	is_k_space = 0;
	if( isfield( params, 'is_k_space' ) )
		is_k_space = str2num( params.is_k_space );
	end

	% x oversampling
	if( isfield( params, 'x_os' ) )

		if( is_k_space == 1 )
			result = fftshift( ifft( ifftshift( result, 2 ), [] ,2 ), 2 );
		end

		x_os = str2num( params.x_os );
		crop_size = floor( size(result,2)/(x_os*2) );
		result = result(:,crop_size:3*crop_size,:,:,:,:,:,:,:,:,:);

		if( is_k_space == 1 )
			result = ifftshift( fft( fftshift( result, 2 ), [] ,2 ), 2 );
		end
	end

	% y oversampling
	if( isfield( params, 'y_os' ) )
		if( is_k_space == 1 )
			result = fftshift( ifft( ifftshift( result, 1 ), [] ,1 ), 1 );
		end

		y_os = str2num( params.y_os );
		crop_size = floor( size(result,1)/(y_os*2) );
		result = result(crop_size:3*crop_size,:,:,:,:,:,:,:,:,:,:);

		if( is_k_space == 1 )
			result = ifftshift( fft( fftshift( result, 1 ), [] ,1 ), 1 );
		end
	end
