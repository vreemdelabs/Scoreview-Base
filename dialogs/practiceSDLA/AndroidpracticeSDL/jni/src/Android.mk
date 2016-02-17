LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH       := ../SDL
SDL_TTF_PATH   := ../SDL2_ttf/
SDL_IMAGE_PATH := ../SDL2_image/
SDL_NET_PATH   := ../SDL2_net/
PROTOBUF_PATH  := ../protobuf/
GLM_PATH       := ../glm-master/
PRACTICE_PATH  := ../../../

# Main directory
PROTOCOL_BUFFERS_PATH  := ../protobuf/src
MESSAGES_PBUFFERS_PATH := ../../../../../messages
MESHLOADER_PATH        := ../../../../../meshloader
COMMON_PATH            := ../../../../../common

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	            $(LOCAL_PATH)/$(SDL_IMAGE_PATH) \
                    $(LOCAL_PATH)/$(SDL_TTF_PATH) \
                    $(LOCAL_PATH)/$(SDL_NET_PATH) \
                    $(LOCAL_PATH)/$(MESSAGES_PBUFFERS_PATH) \
                    $(LOCAL_PATH)/$(MESHLOADER_PATH) \
		    $(LOCAL_PATH)/$(PROTOBUF_PATH)/ \
                    $(LOCAL_PATH)/$(COMMON_PATH)/ \
                    $(LOCAL_PATH)/$(PROTOCOL_BUFFERS_PATH)/ \
		    $(LOCAL_PATH)/$(GLM_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := \
		    $(SDL_PATH)/src/main/android/SDL_android_main.c \
                    $(COMMON_PATH)/glShaders.cpp \
                    $(COMMON_PATH)/gl2Dprimitives.cpp \
                    $(PRACTICE_PATH)fingerender.cpp \
                    $(PRACTICE_PATH)vrender.cpp \
                    $(PRACTICE_PATH)prenderer.cpp \
                    $(PRACTICE_PATH)grender.cpp \
                    $(PRACTICE_PATH)app.cpp \
                    $(PRACTICE_PATH)appmessages.cpp \
                    $(PRACTICE_PATH)appmouse.cpp \
                    $(PRACTICE_PATH)callbacks.cpp \
                    $(PRACTICE_PATH)sdlcalls.cpp \
                    $(PRACTICE_PATH)threads.cpp \
                    $(PRACTICE_PATH)tcpclientSDLnet.cpp \
                    $(PRACTICE_PATH)main.cpp \
	            $(MESSAGES_PBUFFERS_PATH)/scoreview.pb.cc \
	            $(MESSAGES_PBUFFERS_PATH)/message_coder.cpp \
	            $(MESSAGES_PBUFFERS_PATH)/message_decoder.cpp \
	            $(MESHLOADER_PATH)/lex.yy.c \
	            $(MESHLOADER_PATH)/yywrap.c \
	            $(MESHLOADER_PATH)/parser.cpp \
	            $(MESHLOADER_PATH)/meshloader.cpp \
                    $(COMMON_PATH)/f2n.cpp \
                    $(COMMON_PATH)/gfxareas.cpp \
                    $(COMMON_PATH)/score.cpp \
                    $(COMMON_PATH)/mesh.cpp \
                    $(COMMON_PATH)/keypress.cpp \
                    $(COMMON_PATH)/pictures.cpp 

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_ttf SDL2_net protobuf
#LOCAL_STATIC_LIBRARIES := 
LOCAL_STATIC_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

LOCAL_CPPFLAGS += -fexceptions

#LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -g
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)

include $(BUILD_SHARED_LIBRARY)

