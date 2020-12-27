#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include <Godot.hpp>
#include <MeshInstance.hpp>
#include <ImageTexture.hpp>
#include <SpatialMaterial.hpp>

#include <linux/videodev2.h>

namespace godot{

class Screen : public MeshInstance{
	GODOT_CLASS(Screen, MeshInstance)

private:
	static const uint32_t WIDTH;
	static const uint32_t HEIGHT;
	int32_t cam;
	v4l2_capability _capability;
	v4l2_format _imageFormat;
	v4l2_requestbuffers _request;
	v4l2_buffer _query;
	v4l2_buffer _frameInfo;
	uint8_t* _frame;

	Ref<Image> _frameBuffer;
	Ref<ImageTexture> _texture;
	Ref<SpatialMaterial> _material;

public:
	static void _register_methods();

	Screen();
	~Screen();

	void _init(); // our initializer called by Godot
	void _ready();
	void _process(float delta);
};

}

#endif
