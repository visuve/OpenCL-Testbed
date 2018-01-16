#include "PCH.hpp"
#include "StopWatch.hpp"

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

namespace
{
    std::string EmbeddedResource(DWORD resourceIdentifier)
    {
        HMODULE moduleHandle = GetModuleHandle(nullptr);
        HRSRC resourceInfo = FindResource(moduleHandle, MAKEINTRESOURCE(resourceIdentifier), L"TEXT");
        HGLOBAL resourceHandle = LoadResource(moduleHandle, resourceInfo);
        DWORD resourceSize = SizeofResource(moduleHandle, resourceInfo);
        LPVOID resourceData = LockResource(resourceHandle);
        std::string result(reinterpret_cast<char*>(resourceData), resourceSize);
        FreeResource(resourceHandle);
        return result;
    }
}

int wmain(/*int argc, wchar_t* argv[], wchar_t* envp[]*/)
{
    try
    {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        if (platforms.empty())
        {
            std::cerr << "No OpenCL platforms found!" << std::endl;
            return 1;
        }

        std::vector<cl::Device> availableDevices;

        for (cl::Platform& platform : platforms)
        {
            try
            {
                std::vector<cl::Device> platformDevices;
                platform.getDevices(CL_DEVICE_TYPE_GPU, &platformDevices);

                for (cl::Device& platformDevice : platformDevices)
                {
                    if (!platformDevice.getInfo<CL_DEVICE_AVAILABLE>())
                    {
                        continue;
                    }

                    availableDevices.push_back(platformDevice);
                }
            }
            catch (...)
            {
                availableDevices.clear();
            }
        }

        if (availableDevices.empty())
        {
            std::cerr << "No available OpenCL devices found!" << std::endl;
            return 1;
        }

        cl::Device& selectedDevice = availableDevices.front();
        cl::Context context = cl::Context(selectedDevice);
        cl::CommandQueue queue(context, selectedDevice);

        std::string source = EmbeddedResource(IDR_OPENCL_CODE);

        cl::Program program(context, cl::Program::Sources(1, std::make_pair(source.c_str(), source.size())));

        try
        {
            program.build(availableDevices);
        }
        catch (const cl::Error& e)
        {
            std::cerr << "Failed to compile OpenCL program! Error:" << e.what() << std::endl;
            std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(selectedDevice) << std::endl;
            return 1;
        }

        cl::Kernel kernel(program, "CalculateCrc32s");

        const uint32_t width = 4; // Null included
        const uint32_t height = 9;
        const uint32_t writeSize = height * width; // sizeof(char) == 1
        const uint32_t readSize = height * sizeof(uint32_t);

        const char words[height][width] = { "abc", "def", "ghi", "jkl", "mno", "pqr", "stu", "wvx", "yz0" };
        uint32_t crcs[height] = { 0 };

        cl::Buffer wordBuffer(context, CL_MEM_READ_ONLY, writeSize);
        cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY, readSize);

        kernel.setArg(0, wordBuffer);
        kernel.setArg(1, width);
        kernel.setArg(2, outputBuffer);

        queue.enqueueWriteBuffer(wordBuffer, CL_TRUE, 0, writeSize, words);

        {
            StopWatch<MicroSeconds> stopWatch;
            queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(height));
            queue.enqueueReadBuffer(outputBuffer, CL_TRUE, 0, readSize, crcs);
        }

        for (uint32_t& crc : crcs)
        {
            std::cout << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase << crc << std::endl;
        }
    }
    catch (const cl::Error& e)
    {
        std::cerr << "OpenCL error: " << e.what() << ", code: " << e.err() << std::endl;
        return 1;
    }

    return 0;
}