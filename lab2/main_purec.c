#include <stdio.h>
#include <stdlib.h>

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
	matrix_value_t* r, const int K)
{
	for(int i_row=0; i_row<K; i_row++)
	{	
		for(int i_column=0; i_column<K; i_column++) 
		{	
			matrix_value_t sum = 0;
		  
			for(int j=0; j<K; j++)
			{
				  sum += *(a + K*i_row+ j) * *(b + K * i_column + j );
			}
			
			INTERNAL_MATRIX_VALUE(r, i_row, i_column) = sum;
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
	
	matrix_value_t* a = (matrix_value_t*) malloc(sizeof(matrix_value_t)*K*K);
	matrix_value_t* b = (matrix_value_t*) malloc(sizeof(matrix_value_t)*K*K);
	matrix_value_t* result = (matrix_value_t*) malloc(sizeof(matrix_value_t)*K*K);
	if( a==NULL || b==NULL || result == NULL ) 
	{
		fprintf(stderr, "Can't allocate memory\n");
		exit(-2);
	}
	multiple_internal_matrix(a, b, result, K);

	free(a);
	free(b);
	free(result);

	return 0;
}

