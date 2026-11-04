#include <cstddef>
#include <iomanip>
#include <ios>
#include <iostream>
#include <libyuv/convert_argb.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdio>

#include <libcamera/libcamera.h>
#include <libcamera/framebuffer.h>
#include <libcamera/request.h>

#include <sys/mman.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <cerrno>

#include <libyuv.h>
#include <pngwriter.h>

using namespace libcamera;
using namespace std::chrono_literals;

// Required to store and share all available cameras through libcamera
static std::shared_ptr<Camera> camera;

/*
Sync CPU to buffer in order to ensure proper cacheing when reading JPEG data

@param fileDescriptor Descriptor of type of image data. Probably equivalent of MJPEG
@param flags Flags of how CPU should sync to buffer
 */
static int syncBuffer(int fileDescriptor, __u64 flags){
    // Create sync object to tell the kernel to sync the buffer. Give it our flags
    dma_buf_sync sync{};
    sync.flags = flags;

    // Send message to kernel. Continue to do so if necessary
    int result;
    do{
        result = ioctl(fileDescriptor, DMA_BUF_IOCTL_SYNC, &sync);
    }
    while(result == -1 && (errno == EINTR || errno == EAGAIN));

    return result;
}

static void writePNG(const unsigned char *data, std::size_t bytesAvailable, int width, int height, std::size_t stride, const char *filename){
    // Use PNGwrite to write RGB info to PNG file
    if(!data || width < 1 || height < 1){
        throw std::runtime_error("Invalid image info");
    }

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    if(stride < rowBytes){
        throw std::runtime_error("Invalid image info: stride too small");
    }

    if(bytesAvailable < rowBytes || static_cast<std::size_t>(height - 1) > (bytesAvailable - rowBytes) / stride){
        throw std::runtime_error("Invalid image info: buffer too small");
    }

    // Write file
    std::cout << "init writer\n";
    pngwriter writer(width, height, 0, filename);

    std::cout << "plot loop\n";
    for(int y = 0; y < height; ++y){
        const unsigned char *row = data + static_cast<std::size_t>(y) * stride;
        
        for(int x = 0; x < width; ++x){
            const unsigned char *pixel = row + static_cast<std::size_t>(x) * 3;
            writer.plot(x + 1, height - y, pixel[0] / 255.0, pixel[1] / 255.0, pixel[2] / 255.0);
        }
    }

    std::cout << "close writer\n";
    writer.close();
}

static void requestComplete(Request *request){
    // Ignore if the request was cancelled
    if(request->status() == Request::RequestCancelled){
        return;
    }

    // Get map of frame buffers
    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();

    // Iterate to access metadata and contents of buffers
    for(auto bufferPair : buffers){
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();

        // Print details
        std::cout << "seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";
        unsigned int nplane = 0;
        for(const FrameMetadata::Plane &plane : metadata.planes()){
            std::cout << plane.bytesused;
            if(++nplane < metadata.planes().size()){
                std::cout << "/";
            }
        }

        std::cout << std::endl;

        std::filesystem::path existingFile = "./capture.png";
        if(!exists(existingFile) && metadata.sequence >= 60){
            const StreamConfiguration &cfg = bufferPair.first->configuration();
            // std::cout << "Format: " << cfg.pixelFormat.toString()
            //     << ", width: " << cfg.size.width
            //     << ", height: " << cfg.size.height
            //     << ", stride: " << cfg.stride
            //     << ", memory planes: " << buffer->planes().size()
            //     << '\n';

            // Get data about storage in memory to prepare to read
            //
            // We're going to need the size of the full image for writing the file. We need the location of the end
            // of the image as well. Then, use the mmap Linux library with a nullptr to get the address of the
            // camera data.
            const FrameBuffer::Plane &plane = buffer->planes()[0];
            const std::size_t imgSize = metadata.planes()[0].bytesused;
            // const size_t mappingEnd = static_cast<std::size_t>(plane.offset) + plane.length;
            std::size_t mappingEnd = 0;

            for(const auto &thisPlane : buffer->planes()){
                if (thisPlane.fd.get() != plane.fd.get()) {
                    throw std::runtime_error(
                        "Planes use different descriptors; map them separately"
                    );
                }

                const std::size_t thisEnd = static_cast<std::size_t>(thisPlane.offset) + thisPlane.length;
                mappingEnd = std::max(mappingEnd, thisEnd);
            }

            void *mapping = mmap(nullptr, mappingEnd, PROT_READ, MAP_SHARED, plane.fd.get(), 0);

            // Check if mapping failed
            if(mapping == MAP_FAILED){
                throw std::runtime_error("Failed to map camera buffer");
            }

            // Finally, get data pointer
            const unsigned char *data = static_cast<const unsigned char *>(mapping) + plane.offset;

            // Other guardrails. If these activate, unmap
            if(imgSize == 0 || imgSize > plane.length){
                munmap(mapping, mappingEnd);
                throw std::runtime_error("Invalid JPEG size");
            }
            if(syncBuffer(plane.fd.get(), DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ) == -1){
                munmap(mapping, mappingEnd);
                throw std::runtime_error("Could not prepare buffer for reading");
            }

            std::cout << "Get image property data\n";
            // Properties of image data
            const unsigned int width = static_cast<int>(cfg.size.width);
            const unsigned int height = static_cast<int>(cfg.size.height);
            const unsigned int stride = static_cast<int>(cfg.stride);
            const unsigned int uvStride = stride / 2;
            
            std::cout << "Get YUV mapings\n";
            // Get mappings of all frames of YUV
            const unsigned char *baseMapping = static_cast<const unsigned char *>(mapping);
            const unsigned char *yData = baseMapping + buffer->planes()[0].offset;
            const unsigned char *uData = baseMapping + buffer->planes()[1].offset;
            const unsigned char *vData = baseMapping + buffer->planes()[2].offset;

            std::cout << "Set RGB Expecteds\n";
            // Expected RGB properties
            const int rgbStride = width * 3;
            std::vector<std::uint8_t> rgbData(static_cast<std::size_t>(rgbStride) * height);

            std::cout << "Convert to RGB\n";
            // Convert YUV to RGB with libyuv
            int rgbConversion = libyuv::I420ToRAW(
                yData, stride,
                uData, uvStride,
                vData, uvStride,
                rgbData.data(), rgbStride,
                width, height
            );

            std::cout << "Sync CPU cash, unmap\n";
            // Sync up cache, done reading data from buffer
            const int syncResult = syncBuffer(plane.fd.get(), DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ);
            const int unmap = munmap(mapping, mappingEnd);

            if(rgbConversion != 0){
                throw std::runtime_error("Error converting YUV to RGB");
            }

            const auto [minIt, maxIt] = std::minmax_element(rgbData.begin(), rgbData.end());
            double sum = 0.0;
            for (std::uint8_t value : rgbData)
                sum += value;
            std::cout << "RGB min: " << static_cast<int>(*minIt)
                << ", max: " << static_cast<int>(*maxIt)
                << ", mean: " << sum / rgbData.size()
                << '\n';

            std::cout << "write png\n";
            writePNG(rgbData.data(), rgbData.size(), width, height, rgbStride, "capture.png");

            // Prepare file
            // std::ofstream outFile("capture.jpg", std::ios::binary);
            // bool fileWritten = false;

            // // Write if we can
            // if(outFile){
            //     outFile.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(imgSize));
            //     outFile.close();
            //     fileWritten = static_cast<bool>(outFile);
            // }

            // if(!fileWritten || syncResult == -1 || unmap == -1){
            //     throw std::runtime_error("Error while saving JPEG file");
            // }
        }
    }

    // Reuse buffers and request
    request->reuse(Request::ReuseBuffers);
    camera->queueRequest(request);
}

int main(){
    // Create CameraManager
    std::unique_ptr<CameraManager> camManager = std::make_unique<CameraManager>();
    camManager->start();

    // Get, print all available cameras
    auto cameras = camManager->cameras();
    std::cout << "Available cameras:" << std::endl;
    for(const auto &camera : cameras){
        std::cout << camera->id() << std::endl;
    }
    std::cout << std::endl;

    // Select first camera
    //
    // Stop with error code if none are available
    if(cameras.empty()){
        std::cout << "No cameras available..." << std::endl;
        camManager->stop();
        return 1;
    }
    std::string camID = cameras[0]->id();
    camera = camManager->get(camID);
    camera->acquire(); // Acquire lock

    // Configure
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({ StreamRole::StillCapture });
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default StillCapture config: " << streamConfig.toString() << std::endl;
    /*
    Height/width adjust:
    streamConfig.size.width = 640;
    streamConfig.size.height = 480;
    */

    config->validate();
    std::cout << "Validated config: " << streamConfig.toString() << std::endl;
    camera->configure(config.get());

    // Delete existing capture file if exists
    std::filesystem::path existingCapture = "./capture.jpg";
    if(exists(existingCapture)){
        std::remove("./capture.jpg");
    }

    // Allocate frame buffer
    auto allocator = std::make_unique<FrameBufferAllocator>(camera);
    for(StreamConfiguration &cfg : *config){
        int ret = allocator->allocate(cfg.stream());
        if(ret < 0){
            std::cerr << "Couldn't allocate buffers\n";
            return -ENOMEM;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream\n";
    }

    // Get frame buffers using stream object, initialize vector to send requests
    Stream *stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);
    std::vector<std::unique_ptr<Request>> requests;

    // Create requests
    for(unsigned int i = 0; i < buffers.size(); ++i){
        std::unique_ptr<Request> request = camera->createRequest();
        if(!request){
            std::cerr << "Can't create request\n";
            return -ENOMEM;
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if(ret < 0){
            std::cerr << "Can't set buffer for request\n";
            return ret;
        }

        requests.push_back(std::move(request));
    }

    // Add connection to a completed request
    camera->requestCompleted.connect(requestComplete);

    // All ready. Get started, queue requests
    camera->start();
    for(std::unique_ptr<Request> &request : requests){
        camera->queueRequest(request.get());
    }

    // libcamera starts in its own thread; let it do its thing before shutting down
    std::this_thread::sleep_for(5000ms);

    // Cleanup
    camera->stop();
    allocator->free(stream);
    allocator.reset();
    camera->release();
    camera.reset();
    camManager->stop();
    return 0;
}