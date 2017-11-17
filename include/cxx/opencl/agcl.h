#pragma once

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <CL/cl.h>

namespace agcl {

// Error checking.  Currently, any error will just throw a runtime exception
inline void cl_check(cl_int rc, int line)
{
  if (rc != CL_SUCCESS)
  {
    std::ostringstream os; os << "OpenCL call failed: " << __FILE__ << ":" << line << "  rc=" << rc;
    throw std::runtime_error(os.str());
  }
}

#define CL_CHECK(rc) cl_check(rc,__LINE__)

class BuildFailure : public std::runtime_error
{
public:
  BuildFailure(const char* text) : std::runtime_error(text) {}
};

// OpenCL context, representing a single device
class Context
{
  cl_device_id m_DeviceID;
  cl_uint      m_NumDevices;
  cl_uint      m_NumPlatforms;
  std::vector<cl_platform_id> m_Platforms;
  cl_context   m_Context;
  cl_command_queue m_CommandQueue;
  std::vector<std::string> m_Devices;
public:
  // Specify a substring name to look for in device vendors/names
  // Examples:  "intel"  "nvidia" ....
  // If not found, or not specified (empty string), first device is used
  Context(std::string name)
  {
    std::transform(name.begin(), name.end(), name.begin(), tolower);
    cl_int rc = clGetPlatformIDs(0, NULL, &m_NumPlatforms);
    CL_CHECK(rc);
    m_Platforms.resize(m_NumPlatforms);
    rc = clGetPlatformIDs(m_NumPlatforms, &m_Platforms[0], NULL);
    CL_CHECK(rc);
    size_t sel = 0;
    for (size_t i = 0; i < m_NumPlatforms; ++i)
    {
      char text[256];
      size_t act;
      std::string platform_name;
      clGetPlatformInfo(m_Platforms[i], CL_PLATFORM_VENDOR, 256, text, &act);
      text[act] = 0;
      platform_name += text;
      platform_name += " ";
      clGetPlatformInfo(m_Platforms[i], CL_PLATFORM_NAME, 256, text, &act);
      text[act] = 0;
      platform_name += text;
      std::transform(platform_name.begin(), platform_name.end(), platform_name.begin(), tolower);
      if (platform_name.find(name) != std::string::npos)
        sel = i;
      m_Devices.push_back(platform_name);
    }

    rc = clGetDeviceIDs(m_Platforms[sel], CL_DEVICE_TYPE_ALL, 1,
                        &m_DeviceID, &m_NumDevices);
    CL_CHECK(rc);
    m_Context = clCreateContext(NULL, 1, &m_DeviceID, NULL, NULL, &rc);
    CL_CHECK(rc);
    m_CommandQueue = clCreateCommandQueue(m_Context, m_DeviceID, 0, &rc);
    CL_CHECK(rc);
  }

  ~Context()
  {
    cl_int rc;
    rc = clFlush(m_CommandQueue);
    rc = clFinish(m_CommandQueue);
    rc = clReleaseContext(m_Context);
  }

  // Iterate over all found devices
  typedef typename std::vector<std::string>::const_iterator const_iterator;
  const_iterator begin() const { return m_Devices.begin(); }
  const_iterator end()   const { return m_Devices.end();   }

  cl_device_id     get_device_id()     { return m_DeviceID;     }
  cl_context       get_context()       { return m_Context;      }
  cl_command_queue get_command_queue() { return m_CommandQueue; }
};

// Generic data buffer that allocates memory on both RAM and VRAM,
// and allows the transfer for GPU compute
template<class T>
class Buffer
{
  typedef typename std::vector<T> cpu_buffer;
  cpu_buffer m_CPU_Buffer;
  cl_mem     m_GPU_Buffer;
  cl_command_queue m_CommandQueue;
  Buffer(const Buffer&) {}
  Buffer& operator=(const Buffer&) { return *this; }
protected:
  void update_gpu_buffer(size_t index=0, size_t count=std::numeric_limits<size_t>::max())
  {
    cl_int rc = clEnqueueWriteBuffer(m_CommandQueue, m_GPU_Buffer, CL_TRUE, 0,
                               m_CPU_Buffer.size() * sizeof(T), get(), 0, NULL, NULL);
    CL_CHECK(rc);
  }

  void update_cpu_buffer(size_t index = 0, size_t count = std::numeric_limits<size_t>::max())
  {
    if ((index + count) > size())
      count = size() - index;
    size_t byte_index = index * sizeof(T);
    size_t byte_count = count * sizeof(T);
    uint8_t* cpu_buf = reinterpret_cast<uint8_t*>(get());
    cl_int rc = clEnqueueReadBuffer(m_CommandQueue, m_GPU_Buffer, CL_TRUE, byte_index,
                                    byte_count, cpu_buf + byte_index, 0, NULL, NULL);
    CL_CHECK(rc);
  }
public:
  Buffer(Context& ctx, size_t n, bool input, bool output)
    : m_CPU_Buffer(n)
    , m_CommandQueue(ctx.get_command_queue())
  {
    cl_int rc;
    int flags = (input? CL_MEM_READ_ONLY:0) | (output ? CL_MEM_WRITE_ONLY : 0);
    if (input && output) flags = CL_MEM_READ_WRITE;
    m_GPU_Buffer = clCreateBuffer(ctx.get_context(), flags,
                                  n * sizeof(T), NULL, &rc);
    CL_CHECK(rc);
  }

  ~Buffer()
  {
    clReleaseMemObject(m_GPU_Buffer);
  }
  
  cl_mem get_gpu_buffer() { return m_GPU_Buffer; }

  size_t size() const { return m_CPU_Buffer.size(); }
  T* get() { return &m_CPU_Buffer[0]; }
  const T* get() const { return &m_CPU_Buffer[0]; }
  T& operator[] (size_t i) { return m_CPU_Buffer[i]; }
  const T& operator[] (size_t i) const { return m_CPU_Buffer[i]; }
};

// Sub-type representing an input buffer, that is filled by CPU code,
// and then sent to the GPU for processing
template<class T>
class InputBuffer : public Buffer<T>
{
public:
  InputBuffer(Context& ctx, size_t n)
    : Buffer<T>(ctx,n,true,false)
  {}

  void update()
  {
    Buffer<T>::update_gpu_buffer();
  }
};

// Sub-type representing a buffer that is calculated by GPU code, 
// and then transferred back to RAM as a result
template<class T>
class OutputBuffer : public Buffer<T>
{
public:
  OutputBuffer(Context& ctx, size_t n)
    : Buffer<T>(ctx, n, false, true)
  {}

  void update(size_t offset=0, size_t count=std::numeric_limits<size_t>::max())
  {
    Buffer<T>::update_cpu_buffer(offset,count);
  }
};

// A single GPU function
class Kernel
{
  cl_command_queue m_CommandQueue;
  cl_kernel m_Kernel;
  size_t    m_BlockSize;
  int       m_Index;

  template<class T>
  void add_argument(Buffer<T>* buffer)
  {
    cl_mem buf = buffer->get_gpu_buffer();
    cl_int rc = clSetKernelArg(m_Kernel, m_Index++, sizeof(cl_mem), (void *)&buf);
    CL_CHECK(rc);
  }

  void add_argument(cl_int val)
  {
    cl_int rc = clSetKernelArg(m_Kernel, m_Index++, sizeof(cl_int), (void *)&val);
    CL_CHECK(rc);
  }

  void add_arguments() {}

  template<typename First, typename ...Rest>
  void add_arguments(First first, Rest... rest)
  {
    add_argument(first);
    add_arguments(rest...);
  }
public:
  Kernel() : m_Kernel(nullptr), m_CommandQueue(nullptr) {}
  Kernel(Context& ctx, cl_kernel k, size_t block_size)
    : m_Kernel(k)
    , m_CommandQueue(ctx.get_command_queue())
    , m_BlockSize(block_size)
  {}

  // Activate the GPU function.  work_items is the total number of elements
  // to process.  The GPU function is called this number of times, each processing
  // a single element
  template<typename ...Ts>
  void operator() (size_t work_items, Ts... args)
  {
    if (!m_Kernel) return;
    m_Index = 0;
    add_arguments(args...);
    cl_int rc = clEnqueueNDRangeKernel(m_CommandQueue, m_Kernel, 1, NULL,
                                       &work_items, &m_BlockSize, 0, NULL, NULL);
    CL_CHECK(rc);
  }
};

// A program composed of potentially multiple GPU functions
class Program
{
  cl_program m_Program;
  Context&   m_Context;
  size_t     m_BlockSize;
  std::unordered_map<std::string, Kernel> m_Kernels;
public:
  Program(Context& ctx, const std::string& code)
    : m_Context(ctx)
    , m_BlockSize(64)
  {
    cl_int rc;
    const char* c_code = &code[0];
    size_t size = code.length();
    m_Program = clCreateProgramWithSource(ctx.get_context(), 1,
                                          &c_code, &size, &rc);
    CL_CHECK(rc);
    cl_device_id dev_id = ctx.get_device_id();
    rc = clBuildProgram(m_Program, 1, &dev_id, NULL, NULL, NULL);
    if (rc == CL_BUILD_PROGRAM_FAILURE)
    {
      char build_log[65536];
      size_t act = 0;
      rc=clGetProgramBuildInfo(m_Program, dev_id, CL_PROGRAM_BUILD_LOG, 65536, build_log, &act);
      CL_CHECK(rc);
      build_log[act] = 0;
      throw BuildFailure(build_log);
    }
    CL_CHECK(rc);
  }

  void set_block_size(size_t block_size)
  {
    m_BlockSize = block_size;
  }

  // Retrieve a single GPU function by name
  Kernel operator[](const std::string& name)
  {
    auto it = m_Kernels.find(name);
    if (it != m_Kernels.end()) return it->second;
    cl_int rc;
    Kernel res(m_Context,clCreateKernel(m_Program, name.c_str(), &rc),m_BlockSize);
    CL_CHECK(rc);
    m_Kernels[name] = res;
    return res;
  }

};


} // namespace agcl
