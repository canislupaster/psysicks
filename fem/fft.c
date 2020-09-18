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

void fft(axis_t* base, axis_t* coeffs, int i, int stride) {

	axis_iter_t iter = axis_iter(base, i);

	while (axis_next(&iter)) {

		iter.indices[0]
	}
}