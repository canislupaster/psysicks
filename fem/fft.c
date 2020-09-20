#include <field.h>
#include <complex.h>

//single axis fft
complex float* fft_single(float* v, int v_stride, complex float* coeffs, int res, int stride) {
	complex float* fftres = malloc(sizeof(complex float)*res);

	if (res % 2 == 0) {
		complex float* fft1 = fft_single(v, v_stride*2, coeffs, res/2, stride*2);
		complex float* fft2 = fft_single(&v[v_stride], v_stride*2, coeffs, res/2, stride*2);

		//pls optimize idk assembeley
		for (int i=0; i<res; i++) {
			fftres[i] = fft1[i%(res/2)] + fft2[i%(res/2)]*coeffs[stride*(i%res)]; //rotation corresponding to the wave width
		}
	} else {
		memset(fftres, 0+0i, sizeof(complex float)*res);

		for (unsigned i=0; i<res; i++) {
			for (unsigned vi=0; vi<res; vi++) {
				fftres[i] += (v[vi*v_stride] * (1+i)) * coeffs[stride*((i*vi) % res)];
			}
		}
	}

	return fftres;
}

// "full" spectral method, fft_single can also approximate by using rows and then solved with a tridiagonal matrix
// see http://farside.ph.utexas.edu/teaching/329/lectures/node67.html
float* fftsimple(axis_t* base, unsigned char part, complex float* coeffs, int stride) {
	//not fast, easier to multiply than iterate axis
	//complexity only goes up by a degree of 3 :)
	//i feel like ive lost the ability to think

	int d = axis_length(base);
	float* fftres = malloc(sizeof(complex float)*d^3);

	axis_iter_t iter = axis_iter(base);

	while (axis_next(&iter)) {
		for (int x=0; x<d; x++) {
			for (int y=0; x<d; x++) {
				for (int z=0; x<d; x++) {
					fftres[x*d^2 + y*d + z] += iter.x[part]*(float)coeffs[x*iter.indices[0]+y*iter.indices[1]+z*iter.indices[2]];
				}
			}
		}
	}

	return fftres;
}

//utility to sample sine waves, then downscaled in recursive fft functions to reduce divisions
complex float* fft_coeffs(unsigned N) {
	complex float* coeffs = malloc(N*sizeof(complex float));
	float fn = (float)N;
	for (unsigned i=0; i<N; i++) {
		coeffs[i] = sinf((float)i/fn);
	}

	return coeffs;
}