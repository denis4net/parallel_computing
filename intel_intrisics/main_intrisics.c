#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>

/*
 * AT&T UNIX assembler style 
 * 1. Operand order: source destination
 * 2. Operand size in opcode: b (byte), w(word), l(long, 32bit)
 * 3. Prefix % for registers
 * 4. Imediate constants by $
 * asm(""
 * 	: output operands
 * 	: input operands
 *  : list of clobbered registers
 * )
 */

/* Задание:
 *
 * 1. Написать программу переумножения двух матриц, при этом размерность матрицы  M*M подбирается т.о,
 * чтобы время переумножения было порядка 3..4с. Элементами матрицы является матрица K*K.
 *
 * 2. Собрать проект с использованием icc с опциями -Qvec -QxSSE2 -Qvec-report3
 */

/* Векторицзация
 * 1. MMX 64bit
 * 2. SSE 128bit
 * 3. AVX 256bit
 */

#define INTERNAL_MATRIX_VALUE(internal_matrix, row, column) *(internal_matrix+row*K+column)
/***********************************************************************/
typedef float matrix_value_t;

/***********************************************************************/	

int multiple_internal_matrix(matrix_value_t *a,
		matrix_value_t* b,
		matrix_value_t* r, const int K){

	__m128 a_line, b_line, r_line;

	for (int row=0; row<K; ++row) {

		for(int column=0; column<K; ++column)
		{
			r_line = _mm_set1_ps(0);

			for(int j=0; j<K; j+=4) {
				a_line = _mm_load_ps(a+row*K+j); // load float packet (128/sizeof(float)=4) to sse register
				b_line = _mm_load_ps(b+column*K+j); // load flaot packet row
				r_line = _mm_add_ps( _mm_mul_ps(a_line, b_line), r_line ); // vectorized multiplication of float packets at sse registers
			}
			_mm_store_ps(r+row*K+column, r_line);
		}
	}

	return 0;
}

int main(int argc, char** argv)
{		

	if(argc!=2) {
		fprintf(stderr, "Count argument didn't passed\n");
		exit(-1);
	}

	const int K = atoi(argv[1]);

	matrix_value_t* a, *b, *result;
	posix_memalign(&a, 16, sizeof(matrix_value_t)*K*K);
	posix_memalign(&b, 16, sizeof(matrix_value_t)*K*K);
	posix_memalign(&result, 16, sizeof(matrix_value_t)*K*K);

	if( a==NULL || b==NULL || result == NULL ) 
	{
		fprintf(stderr, "Can't allocate memory\n");
		exit(-2);
	}
#if 0
	for(int i=0; i<K*K; i++)
		scanf("%f", a+i);

	for(int i=0; i<K*K; i++)
		scanf("%f", b+i);
#endif

	multiple_internal_matrix(a, b, result, K);
#if 0
	for(int i=0; i<K*K; i++)
		scanf("%f ", result+i);
#endif
	free(a);
	free(b);
	free(result);

	return 0;
}

