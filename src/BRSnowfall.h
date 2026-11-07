#pragma once

#include <FFGLSDK.h>
#include <chrono>
#include <string>
#include <vector>

class BRSnowfall final : public CFFGLPlugin {
public:
  BRSnowfall();
  ~BRSnowfall() override = default;

  FFResult InitGL(const FFGLViewportStruct* viewport) override;
  FFResult ProcessOpenGL(ProcessOpenGLStruct* process) override;
  FFResult DeInitGL() override;
  FFResult SetFloatParameter(unsigned int index, float value) override;
  float GetFloatParameter(unsigned int index) override;
  FFResult SetTextParameter(unsigned int index, const char* value) override;
  char* GetTextParameter(unsigned int index) override;

private:
  void addOption(unsigned int parameter, unsigned int element, const char* name, float value);

  std::vector<float> values_;
  const std::string credit_ = "Effect by Belajar Resolume";
  unsigned int viewportWidth_ = 1920;
  unsigned int viewportHeight_ = 1080;
  std::chrono::steady_clock::time_point lastFrame_;
  double elapsed_ = 0.0;
  bool paused_ = false;

  ffglex::FFGLShader shader_;
  ffglex::FFGLScreenQuad quad_;
};

