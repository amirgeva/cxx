#include <cxx/opencl/agcl.h>

const char* program_code = R"(
__kernel void vector_add(__global const int *A, __global const int *B, __global int *C, int n) 
{
  int i = get_global_id(0);
  if (i<n)
    C[i] = A[i] + B[i]; 
}

__kernel void vector_sub(__global const int *A, __global const int *B, __global int *C) 
{
  int i = get_global_id(0);
  C[i] = A[i] - B[i]; 
}
)";

const int N = 256;

int main(int argc, char* argv[])
{
  try
  {
    // Initialize, and select GPU with name substring
    agcl::Context ctx("nvidia");
    // Create program and compile.  Will throw if compile fails
    agcl::Program program(ctx, program_code);

    // Create a cpu/gpu buffer as input to the code
    agcl::InputBuffer<int> A(ctx, N);
    // Create a cpu/gpu buffer as input to the code
    agcl::InputBuffer<int> B(ctx, N);
    // Create a cpu/gpu buffer for the output
    agcl::OutputBuffer<int> C(ctx, N);

    // Fill the inputs
    for (int i = 0; i < N; ++i)
    {
      A[i] = 365 - i;
      B[i] = i;
    }
    A.update(); // Copy from CPU to GPU
    B.update(); // Copy from CPU to GPU

    program["vector_sub"](N, &A, &B, &C); // C = A - B
    C.update(); // Copy result from GPU to CPU
    for (int i = 0; i < N; ++i) std::cout << C[i] << ' ';
    std::cout << std::endl;

    program["vector_add"](N, &A, &B, &C, 7); // C = A + B
    C.update();
    for (int i = 0; i < N; ++i) std::cout << C[i] << ' ';
    std::cout << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}

