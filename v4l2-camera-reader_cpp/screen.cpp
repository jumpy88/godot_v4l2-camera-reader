#include "screen.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace godot;

const uint32_t Screen::WIDTH = 640;
const uint32_t Screen::HEIGHT = 480;

void Screen::_register_methods(){
	register_method("_ready", &Screen::_ready);
	register_method("_process", &Screen::_process);
}

Screen::Screen():
	cam(open("/dev/video0", O_RDWR)),
	_request({}),
	_query({}){
	ioctl(cam, VIDIOC_QUERYCAP, &_capability);
	_imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	_imageFormat.fmt.pix.width = 640;
	_imageFormat.fmt.pix.height = 480;
	_imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	_imageFormat.fmt.pix.field = V4L2_FIELD_NONE;

	_request.count = 1;
	_request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	_request.memory = V4L2_MEMORY_MMAP;
	ioctl(cam, VIDIOC_REQBUFS, &_request);

	_query.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	_query.memory = V4L2_MEMORY_MMAP;
	_query.index = 0;
	ioctl(cam, VIDIOC_QUERYBUF, &_query);

	_frame = static_cast<uint8_t*>(mmap(NULL, _query.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam, _query.m.offset));
	memset(_frame, 0, _query.length);

	memset(&_frameInfo, 0, sizeof(_frameInfo));
	_frameInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	_frameInfo.memory = V4L2_MEMORY_MMAP;
	_frameInfo.index = 0;

	ioctl(cam, VIDIOC_STREAMON, &(_frameInfo.type));
}

Screen::~Screen(){
	// add your cleanup here
	ioctl(cam, VIDIOC_STREAMOFF, &(_frameInfo.type));
	close(cam);
}

void Screen::_init(){
	// initialize any variables here
	_frameBuffer = Ref<Image>(Image::_new());
	_frameBuffer->create(WIDTH, HEIGHT, true, Image::FORMAT_RGB8);
	_frameBuffer->fill(Color(0.54, 0.17, 0.89, 1));
	_frameBuffer->lock();
	for(int32_t i = 0; i < _frameBuffer->get_width(); ++i){
		_frameBuffer->set_pixel(i, 0, Color(1, 0, 0));
		_frameBuffer->set_pixel(i, 1, Color(1, 0, 0));
		_frameBuffer->set_pixel(i, 2, Color(1, 0, 0));
		_frameBuffer->set_pixel(i, HEIGHT - 1, Color(1, 0, 0));
		_frameBuffer->set_pixel(i, HEIGHT - 2, Color(1, 0, 0));
		_frameBuffer->set_pixel(i, HEIGHT - 3, Color(1, 0, 0));
	}
	for(int32_t i = 0; i < _frameBuffer->get_height(); ++i){
		_frameBuffer->set_pixel(0, i, Color(1, 0, 0));
		_frameBuffer->set_pixel(1, i, Color(1, 0, 0));
		_frameBuffer->set_pixel(2, i, Color(1, 0, 0));
		_frameBuffer->set_pixel(WIDTH - 1, i, Color(1, 0, 0));
		_frameBuffer->set_pixel(WIDTH - 2, i, Color(1, 0, 0));
		_frameBuffer->set_pixel(WIDTH - 3, i, Color(1, 0, 0));
	}
	_frameBuffer->unlock();

	_texture = Ref<ImageTexture>(ImageTexture::_new());
	_texture->create_from_image(_frameBuffer);

	_material = Ref<SpatialMaterial>(SpatialMaterial::_new());
	_material->set_texture(SpatialMaterial::TEXTURE_ALBEDO, _texture);

	set_surface_material(0, _material);
}

void Screen::_ready(){
}

void Screen::_process(float delta){
	(void)delta;
	ioctl(cam, VIDIOC_QBUF, &_frameInfo);
	ioctl(cam, VIDIOC_DQBUF, &_frameInfo);

	PoolByteArray buffer;
	buffer.resize(WIDTH * HEIGHT * 3);
	PoolByteArray::Write bufferWriter = buffer.write();

	for(uint32_t i = 0; i < HEIGHT; ++i){
		for(uint32_t j = 0; j < WIDTH; j += 2){
			// See https://www.kernel.org/doc/html/v4.12/media/uapi/v4l/pixfmt-yuyv.html for data access
			uint8_t y1 = _frame[(WIDTH * 2) * i + 2 * j];
			uint8_t u  = _frame[(WIDTH * 2) * i + 2 * j + 1];
			uint8_t y2 = _frame[(WIDTH * 2) * i + 2 * j + 2];
			uint8_t v  = _frame[(WIDTH * 2) * i + 2 * j + 3];

			// See https://www.fourcc.org/fccyvrgb.php for color conversion
//			bufferWriter[(WIDTH * 3) * i + 3 * j] =     1.164 * (y1 - 16)                     + 1.596 * (v - 128);
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 1] = 1.164 * (y1 - 16) - 0.391 * (u - 128) - 0.813 * (v - 128);
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 2] = 1.164 * (y1 - 16) + 2.018 * (u - 128);

//			bufferWriter[(WIDTH * 3) * i + 3 * j + 3] = 1.164 * (y2 - 16)                     + 1.596 * (v - 128);
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 4] = 1.164 * (y2 - 16) - 0.391 * (u - 128) - 0.813 * (v - 128);
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 5] = 1.164 * (y2 - 16) + 2.018 * (u - 128);

			bufferWriter[(WIDTH * 3) * i + 3 * j] =     y1                       + 1.402   * (v - 128);
			bufferWriter[(WIDTH * 3) * i + 3 * j + 1] = y1 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
			bufferWriter[(WIDTH * 3) * i + 3 * j + 2] = y1 + 1.772   * (u - 128);

			bufferWriter[(WIDTH * 3) * i + 3 * j + 3] = y2                       + 1.402   * (v - 128);
			bufferWriter[(WIDTH * 3) * i + 3 * j + 4] = y2 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
			bufferWriter[(WIDTH * 3) * i + 3 * j + 5] = y2 + 1.772   * (u - 128);

//			int32_t r = y1                       + 1.402   * (v - 128);
//			int32_t g = y1 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
//			int32_t b = y1 + 1.772   * (u - 128);
//			bufferWriter[(WIDTH * 3) * i + 3 * j]     = (((r < 0) ? 0 : r) > 255) ? 255 : r;
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 1] = (((g < 0) ? 0 : g) > 255) ? 255 : g;
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 2] = (((b < 0) ? 0 : b) > 255) ? 255 : b;

//			r = y2                       + 1.402   * (v - 128);
//			g = y2 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
//			b = y2 + 1.772   * (u - 128);
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 3] = (((r < 0) ? 0 : r) > 255) ? 255 : r;
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 4] = (((g < 0) ? 0 : g) > 255) ? 255 : g;
//			bufferWriter[(WIDTH * 3) * i + 3 * j + 5] = (((b < 0) ? 0 : b) > 255) ? 255 : b;
		}
	}

	_frameBuffer->create_from_data(WIDTH, HEIGHT, false, Image::FORMAT_RGB8, buffer);
	_texture->set_data(_frameBuffer);
}
