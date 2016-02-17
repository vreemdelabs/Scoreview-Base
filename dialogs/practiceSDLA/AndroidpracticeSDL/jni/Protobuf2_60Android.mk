# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src
LOCAL_MODULE := libprotobuf

LOCAL_SRC_FILES :=  \
src/google/protobuf/descriptor.cc \
src/google/protobuf/descriptor.pb.cc \
src/google/protobuf/descriptor_database.cc \
src/google/protobuf/dynamic_message.cc \
src/google/protobuf/extension_set.cc \
src/google/protobuf/extension_set_heavy.cc \
src/google/protobuf/generated_message_reflection.cc \
src/google/protobuf/generated_message_util.cc \
src/google/protobuf/message.cc \
src/google/protobuf/message_lite.cc \
src/google/protobuf/reflection_ops.cc \
src/google/protobuf/repeated_field.cc \
src/google/protobuf/service.cc \
src/google/protobuf/text_format.cc \
src/google/protobuf/unknown_field_set.cc \
src/google/protobuf/wire_format.cc \
src/google/protobuf/wire_format_lite.cc \
src/google/protobuf/io/coded_stream.cc \
src/google/protobuf/io/gzip_stream.cc \
src/google/protobuf/io/printer.cc \
src/google/protobuf/io/tokenizer.cc \
src/google/protobuf/io/strtod.cc \
src/google/protobuf/io/zero_copy_stream.cc \
src/google/protobuf/io/zero_copy_stream_impl.cc \
src/google/protobuf/io/zero_copy_stream_impl_lite.cc \
src/google/protobuf/stubs/common.cc \
src/google/protobuf/stubs/once.cc \
src/google/protobuf/stubs/structurally_valid.cc \
src/google/protobuf/stubs/strutil.cc \
src/google/protobuf/stubs/substitute.cc \
src/google/protobuf/stubs/stringprintf.cc \
src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc

LOCAL_CPP_EXTENSION := .cc

LOCAL_SHARED_LIBRARIES := \
libz libcutils libutils
LOCAL_LDLIBS := -lz -lm

LOCAL_CFLAGS := -Wno-psabi -frtti -c
LOCAL_CPPFLAGS := -Wno-psabi -frtti -c

include $(BUILD_SHARED_LIBRARY)
 