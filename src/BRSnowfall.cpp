#include "BRSnowfall.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace ffglex;

namespace {
enum Param : FFUInt32 {
  P_START,
  P_PAUSE,
  P_RESUME,
  P_STOP,
  P_RESET,
  P_START_FILL_TIME,
  P_STOP_CLEAR_TIME,
  P_SNOW_TYPE,
  P_AMOUNT,
  P_SIZE,
  P_SIZE_RANDOM,
  P_SPEED,
  P_SPEED_RANDOM,
  P_FALL_DIRECTION,
  P_WIND_DIRECTION,
  P_WIND_STRENGTH,
  P_DRIFT,
  P_ROTATION,
  P_DEPTH,
  P_DOF_ENABLE,
  P_FOCUS_LAYER,
  P_DOF_BLUR,
  P_OPACITY,
  P_HUE,
  P_SATURATION,
  P_BRIGHTNESS,
  P_GLOW,
  P_SOFTNESS,
  P_QUALITY,
  P_SEED,
  P_CREDIT,
  P_COUNT
};

static CFFGLPluginInfo PluginInfo(
  PluginFactory<BRSnowfall>,
  "BRSF",
  "BR Snowfall",
  2,
  1,
  0,
  103,
  FF_SOURCE,
  "GPU snowfall source with round and crystal snow",
  "Effect by Belajar Resolume");

const char* vertexShader = R"GLSL(#version 410 core
layout(location=0) in vec4 vPosition;
layout(location=1) in vec2 vUV;
out vec2 uv;
void main(){
  gl_Position=vPosition;
  uv=vUV;
}
)GLSL";

const char* fragmentShader = R"GLSL(#version 410 core
uniform vec2 resolution;
uniform float elapsed;
uniform float snowType;
uniform float amount;
uniform float snowSize;
uniform float sizeRandom;
uniform float fallSpeed;
uniform float speedRandom;
uniform float fallDirection;
uniform float windDirection;
uniform float windStrength;
uniform float driftAmount;
uniform float rotationSpeed;
uniform float depthAmount;
uniform float dofEnabled;
uniform float focusLayer;
uniform float dofBlur;
uniform float globalOpacity;
uniform vec3 snowColor;
uniform float glowAmount;
uniform float softness;
uniform float quality;
uniform float randomSeed;
uniform float startProgress;
uniform float stopProgress;
in vec2 uv;
out vec4 fragColor;

float hash11(float p){
  p=fract(p*0.1031);
  p*=p+33.33;
  p*=p+p;
  return fract(p);
}

vec2 hash21(float p){
  vec3 p3=fract(vec3(p)*vec3(0.1031,0.1030,0.0973));
  p3+=dot(p3,p3.yzx+33.33);
  return fract((p3.xx+p3.yz)*p3.zy);
}

float lineDistance(vec2 p,vec2 a,vec2 b){
  vec2 pa=p-a;
  vec2 ba=b-a;
  float h=clamp(dot(pa,ba)/dot(ba,ba),0.0,1.0);
  return length(pa-ba*h);
}

float crystalDistance(vec2 p,float angle){
  float c=cos(angle),s=sin(angle);
  p=mat2(c,-s,s,c)*p;
  float d=100.0;
  for(int i=0;i<3;++i){
    float a=float(i)*1.0471975512;
    vec2 axis=vec2(cos(a),sin(a));
    d=min(d,lineDistance(p,-axis*0.92,axis*0.92));
    vec2 side=vec2(-axis.y,axis.x);
    d=min(d,lineDistance(p,axis*0.30,axis*0.58+side*0.26));
    d=min(d,lineDistance(p,axis*0.30,axis*0.58-side*0.26));
    d=min(d,lineDistance(p,axis*0.54,axis*0.78+side*0.18));
    d=min(d,lineDistance(p,axis*0.54,axis*0.78-side*0.18));
    d=min(d,lineDistance(p,-axis*0.30,-axis*0.58+side*0.26));
    d=min(d,lineDistance(p,-axis*0.30,-axis*0.58-side*0.26));
    d=min(d,lineDistance(p,-axis*0.54,-axis*0.78+side*0.18));
    d=min(d,lineDistance(p,-axis*0.54,-axis*0.78-side*0.18));
  }
  return d;
}

vec2 directionVector(float mode){
  if(mode<0.5)return vec2(0.0,-1.0);
  if(mode<1.5)return vec2(0.0,1.0);
  if(mode<2.5)return vec2(-1.0,0.0);
  if(mode<3.5)return vec2(1.0,0.0);
  if(mode<4.5)return normalize(vec2(-0.55,-1.0));
  return normalize(vec2(0.55,-1.0));
}

float travelPosition(vec2 coordinate,float mode){
  if(mode<0.5)return 1.0-coordinate.y;
  if(mode<1.5)return coordinate.y;
  if(mode<2.5)return 1.0-coordinate.x;
  if(mode<3.5)return coordinate.x;
  if(mode<4.5)return clamp((1.0-coordinate.y+1.0-coordinate.x)*0.5,0.0,1.0);
  return clamp((1.0-coordinate.y+coordinate.x)*0.5,0.0,1.0);
}

float renderLayer(vec2 scene,float layerIndex){
  float depth=mix(0.62,1.22,layerIndex/2.0);
  float density=mix(4.0,17.0,amount)*depth;
  vec2 grid=scene*density;
  float baseSpeed=mix(0.035,0.48,fallSpeed)*depth;
  vec2 direction=directionVector(fallDirection);
  vec2 motion=direction*elapsed*baseSpeed;
  motion.x+=windDirection*windStrength*elapsed*0.22*depth;
  grid-=motion*density;
  vec2 base=floor(grid);
  vec2 local=fract(grid);
  float alpha=0.0;

  for(int oy=-1;oy<=1;++oy){
    for(int ox=-1;ox<=1;++ox){
      vec2 offset=vec2(float(ox),float(oy));
      vec2 id=base+offset;
      float identity=id.x*127.1+id.y*311.7+layerIndex*71.9+randomSeed*991.0;
      vec2 randomPoint=0.12+hash21(identity)*0.76;
      float speedVariation=mix(1.0-speedRandom*0.65,1.0+speedRandom*0.65,hash11(identity+4.2));
      randomPoint.y=fract(randomPoint.y+elapsed*baseSpeed*(speedVariation-1.0)*density);
      randomPoint.x+=sin(elapsed*(0.35+hash11(identity+9.7)*1.25)+identity)*driftAmount*0.32;

      vec2 p=local-(offset+randomPoint);
      float randomScale=mix(1.0-sizeRandom*0.72,1.0+sizeRandom*0.85,hash11(identity+15.0));
      bool useCrystal=snowType>0.5&&snowType<1.5;
      if(snowType>1.5)useCrystal=hash11(identity+38.0)>0.58;
      float radius=mix(0.055,0.31,snowSize)*randomScale*depth*(useCrystal?1.32:1.0);
      vec2 shapePoint=p/max(radius,0.001);
      float roundD=length(shapePoint)-1.0;
      float angle=elapsed*rotationSpeed*mix(-2.5,2.5,hash11(identity+27.0))+identity;
      float crystalD=crystalDistance(shapePoint,angle)-0.072;
      float selectedD=useCrystal?crystalD:roundD;

      float focusDistance=0.0;
      if(focusLayer<2.5)focusDistance=abs(layerIndex-focusLayer)/2.0;
      float layerBlur=dofEnabled>0.5?focusDistance*dofBlur:0.0;
      float edge=mix(0.018,0.22,softness)+layerBlur*0.34;
      float flake=1.0-smoothstep(-edge,edge,selectedD);
      float blurHalo=(1.0-smoothstep(0.15,2.05+layerBlur*2.5,length(shapePoint)))*layerBlur*0.42;
      float halo=(1.0-smoothstep(0.0,1.65,length(shapePoint)))*glowAmount*0.42+blurHalo;
      float depthFade=mix(0.52,1.0,depthAmount*layerIndex/2.0+0.2);
      alpha=max(alpha,(flake+halo)*depthFade*mix(1.0,0.68,layerBlur));
    }
  }
  return clamp(alpha,0.0,1.0);
}

void main(){
  float aspect=resolution.x/max(resolution.y,1.0);
  vec2 scene=vec2(uv.x*aspect,uv.y);
  float alpha=renderLayer(scene,0.0);
  if(quality>0.25)alpha=max(alpha,renderLayer(scene,1.0));
  if(quality>0.75)alpha=max(alpha,renderLayer(scene,2.0));
  float travel=travelPosition(uv,fallDirection);
  float startMask=1.0-smoothstep(startProgress,startProgress+0.035,travel);
  float stopMask=smoothstep(stopProgress-0.035,stopProgress,travel);
  alpha*=startMask*stopMask;
  alpha=clamp(alpha*globalOpacity,0.0,1.0);
  fragColor=vec4(snowColor*alpha,alpha);
}
)GLSL";
}

BRSnowfall::BRSnowfall() : values_(P_COUNT,0.0f) {
  SetMinInputs(0);
  SetMaxInputs(0);

  values_[P_SNOW_TYPE]=2.0f;
  values_[P_START_FILL_TIME]=0.35f;
  values_[P_STOP_CLEAR_TIME]=0.35f;
  values_[P_AMOUNT]=0.48f;
  values_[P_SIZE]=0.38f;
  values_[P_SIZE_RANDOM]=0.62f;
  values_[P_SPEED]=0.34f;
  values_[P_SPEED_RANDOM]=0.45f;
  values_[P_FALL_DIRECTION]=0.0f;
  values_[P_WIND_DIRECTION]=0.5f;
  values_[P_WIND_STRENGTH]=0.12f;
  values_[P_DRIFT]=0.35f;
  values_[P_ROTATION]=0.28f;
  values_[P_DEPTH]=0.75f;
  values_[P_DOF_ENABLE]=0.0f;
  values_[P_FOCUS_LAYER]=3.0f;
  values_[P_DOF_BLUR]=0.45f;
  values_[P_OPACITY]=0.92f;
  values_[P_SATURATION]=0.04f;
  values_[P_BRIGHTNESS]=1.0f;
  values_[P_GLOW]=0.25f;
  values_[P_SOFTNESS]=0.30f;
  values_[P_QUALITY]=1.0f;
  values_[P_SEED]=0.37f;

  SetParamInfof(P_START,"Start",FF_TYPE_EVENT);
  SetParamInfof(P_PAUSE,"Pause",FF_TYPE_EVENT);
  SetParamInfof(P_RESUME,"Resume",FF_TYPE_EVENT);
  SetParamInfof(P_STOP,"Stop - Clear Naturally",FF_TYPE_EVENT);
  SetParamInfof(P_RESET,"Reset",FF_TYPE_EVENT);
  SetParamInfof(P_START_FILL_TIME,"Start Fill Time",FF_TYPE_STANDARD);
  SetParamInfof(P_STOP_CLEAR_TIME,"Stop Clear Time",FF_TYPE_STANDARD);

  SetOptionParamInfo(P_SNOW_TYPE,"Snow Type",3,2);
  addOption(P_SNOW_TYPE,0,"Round",0.0f);
  addOption(P_SNOW_TYPE,1,"Crystal",1.0f);
  addOption(P_SNOW_TYPE,2,"Mixed",2.0f);
  SetParamInfof(P_AMOUNT,"Snow Amount",FF_TYPE_STANDARD);
  SetParamInfof(P_SIZE,"Snow Size",FF_TYPE_STANDARD);
  SetParamInfof(P_SIZE_RANDOM,"Size Random",FF_TYPE_STANDARD);
  SetParamInfof(P_SPEED,"Fall Speed",FF_TYPE_STANDARD);
  SetParamInfof(P_SPEED_RANDOM,"Speed Random",FF_TYPE_STANDARD);

  SetOptionParamInfo(P_FALL_DIRECTION,"Fall Direction",6,0);
  addOption(P_FALL_DIRECTION,0,"Down",0.0f);
  addOption(P_FALL_DIRECTION,1,"Up",1.0f);
  addOption(P_FALL_DIRECTION,2,"Left",2.0f);
  addOption(P_FALL_DIRECTION,3,"Right",3.0f);
  addOption(P_FALL_DIRECTION,4,"Down Left",4.0f);
  addOption(P_FALL_DIRECTION,5,"Down Right",5.0f);

  SetParamInfof(P_WIND_DIRECTION,"Wind Direction",FF_TYPE_STANDARD);
  SetParamInfof(P_WIND_STRENGTH,"Wind Strength",FF_TYPE_STANDARD);
  SetParamInfof(P_DRIFT,"Horizontal Drift",FF_TYPE_STANDARD);
  SetParamInfof(P_ROTATION,"Rotation Speed",FF_TYPE_STANDARD);
  SetParamInfof(P_DEPTH,"Depth",FF_TYPE_STANDARD);
  SetParamInfof(P_DOF_ENABLE,"Depth of Field",FF_TYPE_BOOLEAN);
  SetOptionParamInfo(P_FOCUS_LAYER,"DOF Focus Layer",4,3);
  addOption(P_FOCUS_LAYER,0,"Background",0.0f);
  addOption(P_FOCUS_LAYER,1,"Middle",1.0f);
  addOption(P_FOCUS_LAYER,2,"Foreground",2.0f);
  addOption(P_FOCUS_LAYER,3,"All Sharp",3.0f);
  SetParamInfof(P_DOF_BLUR,"DOF Blur Amount",FF_TYPE_STANDARD);

  SetParamInfof(P_OPACITY,"Opacity",FF_TYPE_STANDARD);
  SetParamInfof(P_HUE,"Snow Color",FF_TYPE_HUE);
  SetParamInfof(P_SATURATION,"Snow Color Sat",FF_TYPE_SATURATION);
  SetParamInfof(P_BRIGHTNESS,"Snow Color Bri",FF_TYPE_BRIGHTNESS);
  SetParamInfof(P_GLOW,"Glow",FF_TYPE_STANDARD);
  SetParamInfof(P_SOFTNESS,"Softness",FF_TYPE_STANDARD);

  SetOptionParamInfo(P_QUALITY,"Quality",3,2);
  addOption(P_QUALITY,0,"Low - 1 Layer",0.0f);
  addOption(P_QUALITY,1,"Medium - 2 Layers",0.5f);
  addOption(P_QUALITY,2,"High - 3 Layers",1.0f);
  SetParamInfof(P_SEED,"Random Seed",FF_TYPE_STANDARD);
  SetParamInfo(P_CREDIT,"Effect by Belajar Resolume",FF_TYPE_TEXT,credit_.c_str());

  for(unsigned int i=P_START;i<=P_STOP_CLEAR_TIME;++i)SetParamGroup(i,"Playback");
  for(unsigned int i=P_SNOW_TYPE;i<=P_SPEED_RANDOM;++i)SetParamGroup(i,"Snow Setting");
  for(unsigned int i=P_FALL_DIRECTION;i<=P_DEPTH;++i)SetParamGroup(i,"Motion");
  for(unsigned int i=P_DOF_ENABLE;i<=P_DOF_BLUR;++i)SetParamGroup(i,"Depth of Field");
  for(unsigned int i=P_OPACITY;i<=P_SOFTNESS;++i)SetParamGroup(i,"Appearance");
  for(unsigned int i=P_QUALITY;i<=P_SEED;++i)SetParamGroup(i,"Performance");
  SetParamGroup(P_CREDIT,"Credit");

  lastFrame_=std::chrono::steady_clock::now();
}

void BRSnowfall::addOption(unsigned int parameter,unsigned int element,const char* name,float value){
  SetParamElementInfo(parameter,element,name,value);
}

FFResult BRSnowfall::InitGL(const FFGLViewportStruct* viewport){
  viewportWidth_=viewport->width;
  viewportHeight_=viewport->height;
  if(!shader_.Compile(vertexShader,fragmentShader)||!quad_.Initialise())return FF_FAIL;
  lastFrame_=std::chrono::steady_clock::now();
  return CFFGLPlugin::InitGL(viewport);
}

FFResult BRSnowfall::ProcessOpenGL(ProcessOpenGLStruct*){
  const auto now=std::chrono::steady_clock::now();
  const double delta=std::chrono::duration<double>(now-lastFrame_).count();
  lastFrame_=now;
  if(!paused_)elapsed_+=std::min(delta,0.1);

  const float startDuration=0.25f+values_[P_START_FILL_TIME]*7.75f;
  const float stopDuration=0.25f+values_[P_STOP_CLEAR_TIME]*7.75f;
  const float startProgress=stopped_?0.0f:std::min(1.0f,float(elapsed_/startDuration));
  float stopProgress=0.0f;
  if(stopping_){
    stopProgress=std::min(1.0f,float((elapsed_-stopStartedAt_)/stopDuration));
    if(stopProgress>=1.0f){stopping_=false;stopped_=true;}
  }else if(stopped_)stopProgress=1.0f;

  float red=1.0f,green=1.0f,blue=1.0f;
  HSVtoRGB(values_[P_HUE],values_[P_SATURATION],values_[P_BRIGHTNESS],red,green,blue);

  ScopedShaderBinding binding(shader_.GetGLID());
  const auto uniform=[&](const char* name){return shader_.FindUniform(name);};
  glUniform2f(uniform("resolution"),float(viewportWidth_),float(viewportHeight_));
  glUniform1f(uniform("elapsed"),float(elapsed_));
  glUniform1f(uniform("snowType"),values_[P_SNOW_TYPE]);
  glUniform1f(uniform("amount"),values_[P_AMOUNT]);
  glUniform1f(uniform("snowSize"),values_[P_SIZE]);
  glUniform1f(uniform("sizeRandom"),values_[P_SIZE_RANDOM]);
  glUniform1f(uniform("fallSpeed"),values_[P_SPEED]);
  glUniform1f(uniform("speedRandom"),values_[P_SPEED_RANDOM]);
  glUniform1f(uniform("fallDirection"),values_[P_FALL_DIRECTION]);
  glUniform1f(uniform("windDirection"),(values_[P_WIND_DIRECTION]-0.5f)*2.0f);
  glUniform1f(uniform("windStrength"),values_[P_WIND_STRENGTH]);
  glUniform1f(uniform("driftAmount"),values_[P_DRIFT]);
  glUniform1f(uniform("rotationSpeed"),values_[P_ROTATION]);
  glUniform1f(uniform("depthAmount"),values_[P_DEPTH]);
  glUniform1f(uniform("dofEnabled"),values_[P_DOF_ENABLE]);
  glUniform1f(uniform("focusLayer"),values_[P_FOCUS_LAYER]);
  glUniform1f(uniform("dofBlur"),values_[P_DOF_BLUR]);
  glUniform1f(uniform("globalOpacity"),values_[P_OPACITY]);
  glUniform3f(uniform("snowColor"),red,green,blue);
  glUniform1f(uniform("glowAmount"),values_[P_GLOW]);
  glUniform1f(uniform("softness"),values_[P_SOFTNESS]);
  glUniform1f(uniform("quality"),values_[P_QUALITY]);
  glUniform1f(uniform("randomSeed"),values_[P_SEED]);
  glUniform1f(uniform("startProgress"),startProgress);
  glUniform1f(uniform("stopProgress"),stopProgress);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
  quad_.Draw();
  glDisable(GL_BLEND);
  return FF_SUCCESS;
}

FFResult BRSnowfall::DeInitGL(){
  shader_.FreeGLResources();
  quad_.Release();
  return FF_SUCCESS;
}

FFResult BRSnowfall::SetFloatParameter(unsigned int index,float value){
  if(index>=P_COUNT)return FF_FAIL;
  values_[index]=value;
  if(index==P_START&&value!=0.0f){elapsed_=0.0;paused_=false;stopping_=false;stopped_=false;lastFrame_=std::chrono::steady_clock::now();}
  if(index==P_PAUSE&&value!=0.0f)paused_=true;
  if(index==P_RESUME&&value!=0.0f&&!stopped_){paused_=false;lastFrame_=std::chrono::steady_clock::now();}
  if(index==P_STOP&&value!=0.0f&&!stopped_&&!stopping_){paused_=false;stopping_=true;stopStartedAt_=elapsed_;lastFrame_=std::chrono::steady_clock::now();}
  if(index==P_RESET&&value!=0.0f){elapsed_=0.0;paused_=false;stopping_=false;stopped_=false;lastFrame_=std::chrono::steady_clock::now();}
  return FF_SUCCESS;
}

float BRSnowfall::GetFloatParameter(unsigned int index){
  return index<P_COUNT?values_[index]:0.0f;
}

FFResult BRSnowfall::SetTextParameter(unsigned int index,const char*){
  return index==P_CREDIT?FF_SUCCESS:FF_FAIL;
}

char* BRSnowfall::GetTextParameter(unsigned int index){
  static thread_local char buffer[256]{};
  if(index!=P_CREDIT)return buffer;
  const size_t length=std::min(credit_.size(),sizeof(buffer)-1);
  std::memcpy(buffer,credit_.data(),length);
  buffer[length]=0;
  return buffer;
}

