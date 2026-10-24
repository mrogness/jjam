#include <iomanip>
#include <iostream>
#include <libcamera/framebuffer.h>
#include <libcamera/request.h>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

// Required to store and share all available cameras through libcamera
static std::shared_ptr<Camera> camera;

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
    std::this_thread::sleep_for(3000ms);

    // Cleanup
    camera->stop();
    allocator->free(stream);
    allocator.reset();
    camera->release();
    camera.reset();
    camManager->stop();
    return 0;
}