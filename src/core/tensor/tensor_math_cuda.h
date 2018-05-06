/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef  SINGA_CORE_TENSOR_TENSOR_MATH_CUDA_H_
#define  SINGA_CORE_TENSOR_TENSOR_MATH_CUDA_H_
#include "singa/singa_config.h"
#ifdef USE_CUDA
#include "singa/core/tensor.h"
#include "./tensor_math.h"
#include "./math_kernel.h"
#include "singa/utils/cuda_utils.h"
#include "singa/core/common.h"
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include "singa/utils/cuda_utils.h"
#include <cudnn.h>

namespace singa {

/// out[i] = |in[i]|
template <>
void Abs<float, lang::Cuda>(const Tensor* in, Tensor* out,
                            Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());

  cudnnOpTensorOp_t op = CUDNN_OP_TENSOR_MAX;
  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
  cudnnOpTensorDescriptor_t op_desc;
  cudnnCreateOpTensorDescriptor(&op_desc);
  cudnnSetOpTensorDescriptor(op_desc, op, cudnn_dtype, cudnn_propagation);
  
  float alpha1[1] = {1.0};
  float alpha2[1] = {-1.0};
  float beta[1] = {0.0};
  cudnnTensorDescriptor_t in_desc, out_desc;
  cudnnCreateTensorDescriptor(&in_desc);
  cudnnCreateTensorDescriptor(&out_desc);
  cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
  cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
  cudnnOpTensor(ctx->cudnn_handle, op_desc, (void*)(&alpha1), in_desc, inPtr, 
                (void*)(&alpha2), in_desc, inPtr, (void*)(&beta), out_desc, outPtr);

  cudnnDestroyTensorDescriptor(in_desc);
  cudnnDestroyTensorDescriptor(out_desc);
}

template <>
void Set<float, lang::Cuda>(const float x, Tensor* out,
                            Context* ctx) {
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  //float valuePtr[1] = {x};

  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnTensorDescriptor_t out_desc;
  cudnnCreateTensorDescriptor(&out_desc);
  cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
  cudnnSetTensor(ctx->cudnn_handle, out_desc, outPtr, (void*)(&x));

  cudnnDestroyTensorDescriptor(out_desc);
}

template <>
void Add<float, lang::Cuda>(const Tensor* in, const float x,
                            Tensor* out, Context* ctx) {
  Set<float, lang::Cuda>(x, out, ctx);
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());

  float alpha = 1.0, beta=1.0;
  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnTensorDescriptor_t in_desc, out_desc;
  cudnnCreateTensorDescriptor(&in_desc);
  cudnnCreateTensorDescriptor(&out_desc);
  cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
  cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
  cudnnAddTensor(ctx->cudnn_handle, (void*)(&alpha), in_desc, inPtr,  (void*)(&beta), out_desc, outPtr);

  cudnnDestroyTensorDescriptor(in_desc);
  cudnnDestroyTensorDescriptor(out_desc);
}

/// out = in1 + in2
template <>
void Add<float, lang::Cuda>(const Tensor* in1,
                            const Tensor* in2, Tensor* out, Context* ctx) {
  const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());

  cudnnOpTensorOp_t op = CUDNN_OP_TENSOR_ADD;
  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
  cudnnOpTensorDescriptor_t op_desc;
  cudnnCreateOpTensorDescriptor(&op_desc);
  cudnnSetOpTensorDescriptor(op_desc, op, cudnn_dtype, cudnn_propagation);

  float alpha1[1] = {1.0};
  float alpha2[1] = {1.0};
  float beta[1] = {0.0};
  cudnnTensorDescriptor_t in1_desc, in2_desc, out_desc;
  cudnnCreateTensorDescriptor(&in1_desc);
  cudnnCreateTensorDescriptor(&in2_desc);
  cudnnCreateTensorDescriptor(&out_desc);
  cudnnSetTensorNdDescriptor(in1_desc, cudnn_dtype, in1->generate_dim_cuda(), in1->generate_shape_cuda().data(), in1->generate_strides_cuda().data());
  if((in1->nDim() == in2->nDim()) || (in2->nDim() == 1)){
    cudnnSetTensorNdDescriptor(in2_desc, cudnn_dtype, in2->generate_dim_cuda(), in2->generate_shape_cuda().data(), in2->generate_strides_cuda().data());
  } else {
    cudnnSetTensorNdDescriptor(in2_desc, cudnn_dtype, in1->generate_dim_cuda(), in1->generate_shape_cuda().data(), in1->generate_strides_cuda().data());
  }

  cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
  cudnnOpTensor(ctx->cudnn_handle, op_desc, (void*)(alpha1), in1_desc, inPtr1,
                (void*)(alpha2), in2_desc, inPtr2, (void*)(beta), out_desc, outPtr);

  cudnnDestroyTensorDescriptor(in1_desc);
  cudnnDestroyTensorDescriptor(in2_desc);
  cudnnDestroyTensorDescriptor(out_desc);
}

/// out = in1 - in2
template <>
void Sub<float, lang::Cuda>(const Tensor* in1,
                            const Tensor* in2, Tensor* out, Context* ctx) {
  const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());

  cudnnOpTensorOp_t op = CUDNN_OP_TENSOR_ADD;
  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
  cudnnOpTensorDescriptor_t op_desc;
  cudnnCreateOpTensorDescriptor(&op_desc);
  cudnnSetOpTensorDescriptor(op_desc, op, cudnn_dtype, cudnn_propagation);

  float alpha1[1] = {1.0};
  float alpha2[1] = {-1.0};
  float beta[1] = {0.0};
  cudnnTensorDescriptor_t in1_desc, in2_desc, out_desc;
  cudnnCreateTensorDescriptor(&in1_desc);
  cudnnCreateTensorDescriptor(&in2_desc);
  cudnnCreateTensorDescriptor(&out_desc);
  cudnnSetTensorNdDescriptor(in1_desc, cudnn_dtype, in1->generate_dim_cuda(), in1->generate_shape_cuda().data(), in1->generate_strides_cuda().data());
  if((in1->nDim() == in2->nDim()) || (in2->nDim() == 1)){
    cudnnSetTensorNdDescriptor(in2_desc, cudnn_dtype, in2->generate_dim_cuda(), in2->generate_shape_cuda().data(), in2->generate_strides_cuda().data());
  } else {
    cudnnSetTensorNdDescriptor(in2_desc, cudnn_dtype, in1->generate_dim_cuda(), in1->generate_shape_cuda().data(), in1->generate_strides_cuda().data());
  }

  cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
  cudnnOpTensor(ctx->cudnn_handle, op_desc, (void*)(alpha1), in1_desc, inPtr1,
                (void*)(alpha2), in2_desc, inPtr2, (void*)(beta),  out_desc, outPtr);

  cudnnDestroyTensorDescriptor(in1_desc);
  cudnnDestroyTensorDescriptor(in2_desc);
  cudnnDestroyTensorDescriptor(out_desc);
}

/// Element-wise operation, clamp every element into [low, high]
/// if x>high, then x=high; if x<low, then x=low.
template <>
void Clamp<float, lang::Cuda>(const float low,
                              const float high, const Tensor* in, Tensor* out,
                              Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::clamp(num, low, high, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}
/// out = in1 / in2
template <>
void Div<float, lang::Cuda>(const Tensor* in1,
                            const Tensor* in2, Tensor* out, Context* ctx) {
  const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in1->Size();

  if(in1->strides() == in2->strides()){ //if both in1 and in2 strides are the same, we proceed to normal cuda::div
        cuda::div(num, inPtr1, inPtr2, outPtr, ctx->stream);
        out->Set_Strides(in1->strides());
  } else { //else we transform in1 to out to store first
    float alpha[1] = {1.0};
    float beta[1] = {0.0};

    cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
    cudnnTensorDescriptor_t in1_desc, out_desc;
    cudnnCreateTensorDescriptor(&in1_desc);
    cudnnCreateTensorDescriptor(&out_desc);
    cudnnSetTensorNdDescriptor(in1_desc, cudnn_dtype, in1->generate_dim_cuda(), in1->generate_shape_cuda().data(), in1->generate_strides_cuda().data());
    out->Set_Strides(in2->strides());
    cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
    cudnnTransformTensor(ctx->cudnn_handle, (void*)(alpha), in1_desc, inPtr1,
                         (void*)(beta), out_desc, outPtr);

    cuda::div(num, outPtr, inPtr2, outPtr, ctx->stream);
    cudnnDestroyTensorDescriptor(in1_desc);
    cudnnDestroyTensorDescriptor(out_desc);
  }
}

template <>
void Div<float, lang::Cuda>(const float x, const Tensor* in,
                            Tensor* out, Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::div(num, x, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}

/// out = in * x
template <>
void EltwiseMult<float, lang::Cuda>(const Tensor* in,
                                    const float x, Tensor* out, Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());

  float alpha = x, beta = 0.0;
  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnTensorDescriptor_t in_desc, out_desc;
  cudnnCreateTensorDescriptor(&in_desc);
  cudnnCreateTensorDescriptor(&out_desc);
  cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
  cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
  cudnnAddTensor(ctx->cudnn_handle, (void*)(&alpha), in_desc, inPtr,  (void*)(&beta), out_desc, outPtr);

  cudnnDestroyTensorDescriptor(in_desc);
  cudnnDestroyTensorDescriptor(out_desc);
}

/// out = in1 * in2
template <>
void EltwiseMult<float, lang::Cuda>(const Tensor* in1,
                                    const Tensor* in2, Tensor* out,
                                    Context* ctx) {
  const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in1->Size();

  if(in1->strides() == in2->strides()){ //if both in1 and in2 strides are the same, we proceed to normal cuda::mult
        cuda::mult(num, inPtr1, inPtr2, outPtr, ctx->stream);
        out->Set_Strides(in1->strides());
  } else { //else we transform in1 to out to store first
    float alpha[1] = {1.0};
    float beta[1] = {0.0};


    cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
    cudnnTensorDescriptor_t in1_desc, out_desc;
    cudnnCreateTensorDescriptor(&in1_desc);
    cudnnCreateTensorDescriptor(&out_desc);
    cudnnSetTensorNdDescriptor(in1_desc, cudnn_dtype, in1->generate_dim_cuda(), in1->generate_shape_cuda().data(), in1->generate_strides_cuda().data());
    out->Set_Strides(in2->strides());
    cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
    cudnnTransformTensor(ctx->cudnn_handle, (void*)(alpha), in1_desc, inPtr1,
                         (void*)(beta), out_desc, outPtr);

    cuda::mult(num, outPtr, inPtr2, outPtr, ctx->stream);
    cudnnDestroyTensorDescriptor(in1_desc);
    cudnnDestroyTensorDescriptor(out_desc);
  }
}


/// Base is e. out[i]=e^in[i]
template <>
void Exp<float, lang::Cuda>(const Tensor* in, Tensor* out,
                            Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::exp(num, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}

template <>
void GE<float, lang::Cuda>(const Tensor* in, const float x,
                           Tensor* out, Context* ctx) {
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const float* inPtr = static_cast<const float*>(in->block()->data());
  const size_t num = in->Size();
  cuda::ge(num, inPtr, x, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}
template <>
void GE<float, lang::Cuda>(const Tensor* in1, const Tensor* in2,
                           Tensor* out, Context* ctx) {
  Sub<float, lang::Cuda>(in1, in2, out, ctx);
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  // const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  // const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  const size_t num = in1->Size();
  //cuda::ge(num, inPtr1, inPtr2, outPtr, ctx->stream);
  cuda::ge(num, outPtr, 0.0, outPtr, ctx->stream);
}


template <>
void GT<float, lang::Cuda>(const Tensor* in, const float x,
                           Tensor* out, Context* ctx) {
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const float* inPtr = static_cast<const float*>(in->block()->data());
  const size_t num = in->Size();
  cuda::gt(num, inPtr, x, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}
template <>
void GT<float, lang::Cuda>(const Tensor* in1, const Tensor* in2,
                           Tensor* out, Context* ctx) {
  Sub<float, lang::Cuda>(in1, in2, out, ctx);
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  // const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  // const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  const size_t num = in1->Size();
  //cuda::gt(num, inPtr1, inPtr2, outPtr, ctx->stream);
  cuda::gt(num, outPtr, 0.0, outPtr, ctx->stream);
}
template <>
void LE<float, lang::Cuda>(const Tensor* in, const float x,
                           Tensor* out, Context* ctx) {
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const float* inPtr = static_cast<const float*>(in->block()->data());
  const size_t num = in->Size();
  cuda::le(num, inPtr, x, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}
template <>
void LE<float, lang::Cuda>(const Tensor* in1, const Tensor* in2,
                           Tensor* out, Context* ctx) {
  Sub<float, lang::Cuda>(in1, in2, out, ctx);
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  // const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  // const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  const size_t num = in1->Size();
  //cuda::le(num, inPtr1, inPtr2, outPtr, ctx->stream);
  cuda::le(num, outPtr, 0.0, outPtr, ctx->stream);
}

/// Natual logarithm, the base is e, Neper number out[i]=ln(in[i]).
template <>
void Log<float, lang::Cuda>(const Tensor* in, Tensor* out,
                            Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::log(num, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}
template <>
void LT<float, lang::Cuda>(const Tensor* in, const float x,
                           Tensor* out, Context* ctx) {
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const float* inPtr = static_cast<const float*>(in->block()->data());
  const size_t num = in->Size();
  cuda::lt(num, inPtr, x, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}
template <>
void LT<float, lang::Cuda>(const Tensor* in1, const Tensor* in2,
                           Tensor* out, Context* ctx) {
  Sub<float, lang::Cuda>(in1, in2, out, ctx);
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  // const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  // const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  const size_t num = in1->Size();
  //cuda::lt(num, inPtr1, inPtr2, outPtr, ctx->stream);
  cuda::lt(num, outPtr, 0.0, outPtr, ctx->stream);
}
/// Element-wise operation, out[i] = in[i]^x
template <>
void Pow<float, lang::Cuda>(const Tensor* in, const float x,
                            Tensor* out, Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::pow(num, inPtr, x, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}
/// Element-wise operation, out[i] = in1[i]^in2[i]
template <>
void Pow<float, lang::Cuda>(const Tensor* in1,
                            const Tensor* in2, Tensor* out, Context* ctx) {
  const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in1->Size();

  if(in1->strides() == in2->strides()){ //if both in1 and in2 strides are the same, we proceed to normal cuda::pow
        cuda::pow(num, inPtr1, inPtr2, outPtr, ctx->stream);
        out->Set_Strides(in1->strides());
  } else { //else we transform in1 to out to store first
    float alpha[1] = {1.0};
    float beta[1] = {0.0};

    cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
    cudnnTensorDescriptor_t in1_desc, out_desc;
    cudnnCreateTensorDescriptor(&in1_desc);
    cudnnCreateTensorDescriptor(&out_desc);
    cudnnSetTensorNdDescriptor(in1_desc, cudnn_dtype, in1->generate_dim_cuda(), in1->generate_shape_cuda().data(), in1->generate_strides_cuda().data());
    out->Set_Strides(in2->strides());
    cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
    cudnnTransformTensor(ctx->cudnn_handle, (void*)(alpha), in1_desc, inPtr1,
                         (void*)(beta), out_desc, outPtr);

    cuda::pow(num, outPtr, inPtr2, outPtr, ctx->stream);
    cudnnDestroyTensorDescriptor(in1_desc);
    cudnnDestroyTensorDescriptor(out_desc);
  }
}

/// Element-wise operation, out[i]=max(0, in[i])
// template <>
// void ReLU<float, lang::Cuda>(const Tensor* in, Tensor* out,
//                              Context* ctx) {
//   const float* inPtr = static_cast<const float*>(in->block()->data());
//   float* outPtr = static_cast<float*>(out->block()->mutable_data());

//   cudnnActivationDescriptor_t act_desc;
//   cudnnActivationMode_t mode = CUDNN_ACTIVATION_RELU;
//   cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
//   double coef = 0.0; //only used for CLIPPED_RELU or ELU
//   cudnnCreateActivationDescriptor(&act_desc);
//   cudnnSetActivationDescriptor(act_desc, mode, cudnn_propagation, coef);
  
//   float alpha[1] = {1.0};
//   float beta[1] = {0.0};
//   cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
//   cudnnTensorDescriptor_t in_desc, out_desc;
//   cudnnCreateTensorDescriptor(&in_desc);
//   cudnnCreateTensorDescriptor(&out_desc);
//   cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
//   cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
//   cudnnActivationForward(ctx->cudnn_handle, act_desc, (void*)(&alpha), in_desc, inPtr, 
//                         (void*)(&beta), out_desc, outPtr);

//   cudnnDestroyTensorDescriptor(in_desc);
//   cudnnDestroyTensorDescriptor(out_desc);
//   cudnnDestroyActivationDescriptor(act_desc);
// }

template <>
void ReLU<float, lang::Cuda>(const Tensor* in, Tensor* out,
                             Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::relu(num, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}

// /// Element-wise operation, out[i]=sigmoid([in[i])
// template <>
// void Sigmoid<float, lang::Cuda>(const Tensor* in, Tensor* out,
//                                 Context* ctx) {
//   const float* inPtr = static_cast<const float*>(in->block()->data());
//   float* outPtr = static_cast<float*>(out->block()->mutable_data());

//   cudnnActivationDescriptor_t act_desc;
//   cudnnActivationMode_t mode = CUDNN_ACTIVATION_SIGMOID;
//   cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
//   double coef = 0.0; //only used for CLIPPED_RELU or ELU
//   cudnnCreateActivationDescriptor(&act_desc);
//   cudnnSetActivationDescriptor(act_desc, mode, cudnn_propagation, coef);
  
//   float alpha[1] = {1.0};
//   float beta[1] = {0.0};
//   cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
//   cudnnTensorDescriptor_t in_desc, out_desc;
//   cudnnCreateTensorDescriptor(&in_desc);
//   cudnnCreateTensorDescriptor(&out_desc);
//   cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
//   cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
//   cudnnActivationForward(ctx->cudnn_handle, act_desc, (void*)(&alpha), in_desc, inPtr, 
//                         (void*)(&beta), out_desc, outPtr);

//   cudnnDestroyTensorDescriptor(in_desc);
//   cudnnDestroyTensorDescriptor(out_desc);
//   cudnnDestroyActivationDescriptor(act_desc);
// }

/// Element-wise operation, out[i]=sigmoid([in[i])
template <>
void Sigmoid<float, lang::Cuda>(const Tensor* in, Tensor* out,
                                Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::sigmoid(num, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}

// out[i] = sign(in[i])
template <>
void Sign<float, lang::Cuda>(const Tensor* in, Tensor* out,
                             Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::sign(num, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}

// Element-wise operation, out[i]=sqrt([in[i])
template <>
void Sqrt<float, lang::Cuda>(const Tensor* in, Tensor* out,
                             Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());

  cudnnOpTensorOp_t op = CUDNN_OP_TENSOR_SQRT;
  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
  cudnnOpTensorDescriptor_t op_desc;
  cudnnCreateOpTensorDescriptor(&op_desc);
  cudnnSetOpTensorDescriptor(op_desc, op, cudnn_dtype, cudnn_propagation);
  
  float alpha1[1] = {1.0};
  float alpha2[1] = {0.0};
  float beta[1] = {0.0};
  cudnnTensorDescriptor_t in_desc, out_desc;
  cudnnCreateTensorDescriptor(&in_desc);
  cudnnCreateTensorDescriptor(&out_desc);
  cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
  cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
  cudnnOpTensor(ctx->cudnn_handle, op_desc, (void*)(&alpha1), in_desc, inPtr, 
                (void*)(&alpha2), in_desc, inPtr, (void*)(&beta), out_desc, outPtr);

  cudnnDestroyTensorDescriptor(in_desc);
  cudnnDestroyTensorDescriptor(out_desc);
}

/// Element-wise operation, out[i]=in[i]^2
template <>
void Square<float, lang::Cuda>(const Tensor* in, Tensor* out,
                               Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::square(num, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}

// template <>
// void Sum<float, lang::Cuda>(const size_t num, const Block* in, float* out,
//                             Context* ctx) {
//   LOG(FATAL) << "Cuda Sum is not implemented!";
//   // const float* inPtr = static_cast<const float*>(in->data());
//   // cuda::sum(num, inPtr, out, ctx->stream);
// }

template <>
void Sum<float, lang::Cuda>(const Tensor* in, float* out,
                            Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());

   //reduce all axes to 1 for cudnnReduce, e.g. Tensor A with shape (2,4) will be reduced to (1)
   Shape reduced_shape = {1};
   Tensor t(reduced_shape, in->device(), in->data_type());
   float* tPtr = static_cast<float*>(t.block()->mutable_data());
   vector<int> reduce_all_axes = in->generate_shape_cuda();
   for (size_t n=0; n<reduce_all_axes.size(); ++n) {
    reduce_all_axes[n] = 1;
   }
   
  //reduce_desc
  cudnnReduceTensorDescriptor_t reduce_desc;
  cudnnReduceTensorOp_t reduce_op = CUDNN_REDUCE_TENSOR_ADD;
  cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
  cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
  cudnnReduceTensorIndices_t cudnn_indices = CUDNN_REDUCE_TENSOR_NO_INDICES;
  cudnnIndicesType_t cudnn_indices_type = CUDNN_32BIT_INDICES;
  cudnnCreateReduceTensorDescriptor(&reduce_desc);
  cudnnSetReduceTensorDescriptor(reduce_desc, reduce_op, cudnn_dtype,
                                 cudnn_propagation, cudnn_indices, cudnn_indices_type);

  //instantiate 2 new tensors to use new blocks as memory instead of cudaMalloc
  Shape reduction_size = {1000};
  Tensor indices(reduction_size, in->device(), in->data_type());
  Tensor workspace(reduction_size, in->device(), in->data_type());
  size_t indices_bytes = indices.block()->size()*1000;
  size_t workspace_bytes = workspace.block()->size()*1000;
  size_t* indicesPtr = static_cast<size_t*>(indices.block()->mutable_data());
  float* workspacePtr = static_cast<float*>(workspace.block()->mutable_data());
  //void* indicesPtr{nullptr}; void* workspacePtr{nullptr};
  //cudaMalloc(&indicesPtr, indices_bytes); cudaMalloc(&workspacePtr, workspace_bytes);

  float alpha[1] = {1.0};
  float beta[1] = {0.0};
  cudnnTensorDescriptor_t in_desc, t_desc;
  cudnnCreateTensorDescriptor(&in_desc);
  cudnnCreateTensorDescriptor(&t_desc);
  cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
  cudnnSetTensorNdDescriptor(t_desc, cudnn_dtype, t.generate_dim_cuda(), reduce_all_axes.data(), reduce_all_axes.data());
  cudnnReduceTensor(ctx->cudnn_handle, reduce_desc,
                    indicesPtr, indices_bytes, workspacePtr, workspace_bytes,
                    (void*)(&alpha), in_desc, inPtr, (void*)(&beta), t_desc, tPtr);

  *out = tPtr[0];
  cudnnDestroyTensorDescriptor(in_desc);
  cudnnDestroyTensorDescriptor(t_desc);
}


/// Element-wise operation, out[i]=tanh([in[i])
// template <>
// void Tanh<float, lang::Cuda>(const Tensor* in, Tensor* out,
//                              Context* ctx) {
//   const float* inPtr = static_cast<const float*>(in->block()->data());
//   float* outPtr = static_cast<float*>(out->block()->mutable_data());

//   cudnnActivationDescriptor_t act_desc;
//   cudnnActivationMode_t mode = CUDNN_ACTIVATION_TANH;
//   cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
//   double coef = 0.0; //only used for CLIPPED_RELU or ELU
//   cudnnCreateActivationDescriptor(&act_desc);
//   cudnnSetActivationDescriptor(act_desc, mode, cudnn_propagation, coef);
  
//   float alpha[1] = {1.0};
//   float beta[1] = {0.0};
//   cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
//   cudnnTensorDescriptor_t in_desc, out_desc;
//   cudnnCreateTensorDescriptor(&in_desc);
//   cudnnCreateTensorDescriptor(&out_desc);
//   cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
//   cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
//   cudnnActivationForward(ctx->cudnn_handle, act_desc, (void*)(&alpha), in_desc, inPtr, 
//                         (void*)(&beta), out_desc, outPtr);

//   cudnnDestroyTensorDescriptor(in_desc);
//   cudnnDestroyTensorDescriptor(out_desc);
//   cudnnDestroyActivationDescriptor(act_desc);
// }

template <>
void Tanh<float, lang::Cuda>(const Tensor* in, Tensor* out,
                                Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = in->Size();
  cuda::tanh(num, inPtr, outPtr, ctx->stream);
  out->Set_Strides(in->strides());
}

// ================Random functions===========================================
/// Each element of out would be 1 with prob p and 0 with 1-p. 0<= p <= 1
// Get the random generator from 'ctx'
// If DType is not float, then convert the threshold to DType
template <>
void Bernoulli<float, lang::Cuda>(const float p, Tensor* out,
                                  Context* ctx) {
  auto rgen = ctx->curand_generator;
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = out->Size();
  CURAND_CHECK(curandGenerateUniform(rgen, outPtr, num));
  cuda::threshold(num, p, outPtr, outPtr, ctx->stream);
}

// The random generator should be extracted from ctx.
// If DType is not float, then convert the low and high to DType
template <>
void Uniform<float, lang::Cuda>(const float low,
                                const float high, Tensor* out, Context* ctx) {
  auto rgen = ctx->curand_generator;
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = out->Size();
  CURAND_CHECK(curandGenerateUniform(rgen, outPtr, num));
  cuda::mult(num, outPtr, high - low, outPtr, ctx->stream);
  cuda::add(num, outPtr, low, outPtr, ctx->stream);
}

// The random generator should be extracted from ctx.
// If DType is not float, then convert the mean and delta to DType
template <>
void Gaussian<float, lang::Cuda>(const float mean,
                                 const float std, Tensor* out, Context* ctx) {
  auto rgen = ctx->curand_generator;
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = out->Size();
  CURAND_CHECK(curandGenerateNormal(rgen, outPtr, num, mean, std));
}

// =========================Blas operations==================================
// ref to http://docs.nvidia.com/cuda/cublas
template <>
void Amax<float, lang::Cuda>(const Tensor* in, size_t* out,
                             Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  int idx = 1;
  const size_t num = in->Size();
  CUBLAS_CHECK(cublasIsamax(handle, num, inPtr, 1, &idx));
  *out = idx - 1;  // cublas index starts from 1
}

/// return the index of the element with the min value.
template <>
void Amin<float, lang::Cuda>(const Tensor* in, size_t* out,
                             Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  int idx = 1;
  const size_t num = in->Size();
  CUBLAS_CHECK(cublasIsamin(handle, num, inPtr, 1, &idx));
  *out = idx - 1;
}

/// out = sum |x| for all x in in
template <>
void Asum<float, lang::Cuda>(const Tensor* in, float* out,
                             Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  const size_t num = in->Size();
  CUBLAS_CHECK(cublasSasum(handle, num, inPtr, 1, out));
}

/// out = alpha * in + out
template <>
void Axpy<float, lang::Cuda>(const float alpha,
                             const Tensor* in, Tensor* out, Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  const size_t num = in->Size();
  CUBLAS_CHECK(cublasSaxpy(handle, num, &alpha, inPtr, 1, outPtr, 1));
}

/// out = \sum_i in1[i] * in2[i]
template <>
void Dot<float, lang::Cuda>(const Tensor* in1,
                            const Tensor* in2, float* out, Context* ctx) {
  const float* inPtr1 = static_cast<const float*>(in1->block()->data());
  const float* inPtr2 = static_cast<const float*>(in2->block()->data());
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  const size_t num = in1->Size();
  CUBLAS_CHECK(cublasSdot(handle, num, inPtr1, 1, inPtr2, 1, out));
}
template <>
void Nrm2<float, lang::Cuda>(const Tensor* in, float* out,
                             Context* ctx) {
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  const float* inPtr = static_cast<const float*>(in->block()->data());
  const size_t num = in->Size();
  cublasSnrm2(handle, num, inPtr, 1, out);
}
template <>
void Scale<float, lang::Cuda>(const float x, Tensor* out,
                              Context* ctx) {
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t num = out->Size();
  CUBLAS_CHECK(cublasSscal(handle, num, &x, outPtr, 1));
}
// NOTE: cublas uses column major order.
// http://peterwittek.com/cublas-matrix-c-style.html
template <>
void DGMM<float, lang::Cuda>(const bool side_right, const Tensor* M, const Tensor* v,
                             Tensor* out, Context* ctx) {
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  const float* MPtr = static_cast<const float*>(M->block()->data());
  const float* vPtr = static_cast<const float*>(v->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t nrow = M->shape(0);
  const size_t ncol = M->shape(1);
  if (side_right) {
    CUBLAS_CHECK(cublasSdgmm(handle, CUBLAS_SIDE_LEFT, ncol, nrow, MPtr, ncol,
                             vPtr, 1, outPtr, ncol));
  } else {
    CUBLAS_CHECK(cublasSdgmm(handle, CUBLAS_SIDE_RIGHT, ncol, nrow, MPtr, ncol,
                             vPtr, 1, outPtr, ncol));
  }
}
template <>
void GEMV<float, lang::Cuda>(const float alpha, const Tensor* A, const Tensor* v,
                             const float beta, Tensor* out, Context* ctx) {
  const float* APtr = static_cast<const float*>(A->block()->data());
  const float* vPtr = static_cast<const float*>(v->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t m = A->shape()[0];
  const size_t n = A->shape()[1];

  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  if (!(A->transpose()))
    CUBLAS_CHECK(cublasSgemv(handle, CUBLAS_OP_T, n, m, &alpha, APtr, n, vPtr,
                             1, &beta, outPtr, 1));
  else
    CUBLAS_CHECK(cublasSgemv(handle, CUBLAS_OP_N, m, n, &alpha, APtr, m, vPtr,
                             1, &beta, outPtr, 1));
}

// http://docs.nvidia.com/cuda/cublas/#cublas-lt-t-gt-gemm
template <>
void GEMM<float, lang::Cuda>(const float alpha,
                             const Tensor* A, const Tensor* B, const float beta,
                             Tensor* C, Context* ctx) {
  auto transA = A->transpose();
  auto transa = transA ? CUBLAS_OP_T : CUBLAS_OP_N;
  auto transB = B->transpose();
  auto transb = transB ? CUBLAS_OP_T : CUBLAS_OP_N;
  const size_t nrowA = A->shape()[0];
  const size_t ncolA = A->shape()[1];
  const size_t ncolB = B->shape()[1];
  int lda = transA ? nrowA : ncolA;
  int ldb = transB ? ncolA : ncolB;
  int ldc = ncolB;
  const float* APtr = static_cast<const float*>(A->block()->data());
  const float* BPtr = static_cast<const float*>(B->block()->data());
  float* CPtr = static_cast<float*>(C->block()->mutable_data());
  auto handle = ctx->cublas_handle;  // TODO(wangwei) set cudastream
  CUBLAS_CHECK(cublasSgemm(handle, transb, transa, ncolB, nrowA, ncolA, &alpha,
                           BPtr, ldb, APtr, lda, &beta, CPtr, ldc));
}

template <>
void ComputeCrossEntropy<float, lang::Cuda>(bool int_target,
                                            const size_t batchsize,
                                            const size_t dim, const Block* p,
                                            const Block* t, Block* loss,
                                            Context* ctx) {
  const float* pPtr = static_cast<const float*>(p->data());
  const int* tPtr = static_cast<const int*>(t->data());
  float* lossPtr = static_cast<float*>(loss->mutable_data());
  cuda::ComputeCrossEntropy(int_target, batchsize, dim, pPtr, tPtr, lossPtr,
      ctx->stream);
}
template <>
void SoftmaxCrossEntropyBwd<float, lang::Cuda>(bool int_target,
                                               const size_t batchsize,
                                               const size_t dim, const Block* p,
                                               const Block* t, Block* grad,
                                               Context* ctx) {
  CHECK_EQ(p, grad) << "Use the same pointer to optimize performance";
  const float* pPtr = static_cast<const float*>(p->data());
  const int* tPtr = static_cast<const int*>(t->data());
  float* gradPtr = static_cast<float*>(grad->mutable_data());
  cuda::SoftmaxCrossEntropyBwd(int_target, batchsize, dim, pPtr, tPtr, gradPtr,
                               ctx->stream);
}

// template <>
// void RowMax<float, lang::Cuda>(const Tensor* in, Tensor* out,
//                                Context* ctx) {
//   const float* inPtr = static_cast<const float*>(in->block()->data());
//   float* outPtr = static_cast<float*>(out->block()->mutable_data());
//   // const size_t nrow = in->shape()[0];
//   // const size_t ncol = in->shape()[1];
//   // cuda::RowMax(nrow, ncol, inPtr, outPtr, ctx->stream);

//   //vector<int> reduce_row_axes_shape = in->generate_shape_cuda();
//   //reduce_row_axes_shape.back() = 1; //reduce axis 1, so we set last element d in shape {a,b,c,d} to 1

//   vector<int> reduce_row_axes_shape = {1,1,1,1};
//   vector<int> reduced_strides = {1,1,1,1};

//   //reduce_desc
//   cudnnReduceTensorDescriptor_t reduce_desc;
//   cudnnReduceTensorOp_t reduce_op = CUDNN_REDUCE_TENSOR_ADD;
//   cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
//   cudnnNanPropagation_t cudnn_propagation = CUDNN_PROPAGATE_NAN;
//   cudnnReduceTensorIndices_t cudnn_indices = CUDNN_REDUCE_TENSOR_NO_INDICES;
//   //cudnnReduceTensorIndices_t cudnn_indices = CUDNN_REDUCE_TENSOR_FLATTENED_INDICES;
//   cudnnIndicesType_t cudnn_indices_type = CUDNN_32BIT_INDICES;
//   cudnnCreateReduceTensorDescriptor(&reduce_desc);
//   cudnnSetReduceTensorDescriptor(reduce_desc, reduce_op, cudnn_dtype,
//                                  cudnn_propagation, cudnn_indices, cudnn_indices_type);

//   //instantiate new tensor to use new blocks as memory instead of cudaMalloc
//   //create 2 tensors of same size as input tensor
//   Shape reduction_size = {1000};
//   Tensor indices(reduction_size, in->device(), in->data_type());
//   Tensor workspace(reduction_size, in->device(), in->data_type());
//   size_t indices_bytes = indices.block()->size()*1000;
//   size_t workspace_bytes = workspace.block()->size()*1000;
//   size_t* indicesPtr = static_cast<size_t*>(indices.block()->mutable_data());
//   float* workspacePtr = static_cast<float*>(workspace.block()->mutable_data());
//   //void* indicesPtr{nullptr}; void* workspacePtr{nullptr};
//   //cudaMalloc(&indicesPtr, indices_bytes); cudaMalloc(&workspacePtr, workspace_bytes);

//   float alpha[1] = {1.0};
//   float beta[1] = {0.0};
//   cudnnTensorDescriptor_t in_desc, out_desc;
//   cudnnCreateTensorDescriptor(&in_desc);
//   cudnnCreateTensorDescriptor(&out_desc);
//   cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
//   //cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), out->generate_shape_cuda().data(), out->generate_strides_cuda().data());
//   cudnnSetTensorNdDescriptor(out_desc, cudnn_dtype, out->generate_dim_cuda(), reduce_row_axes_shape.data(), reduced_strides.data());
//   cudnnReduceTensor(ctx->cudnn_handle, reduce_desc,
//                     indicesPtr, indices_bytes, workspacePtr, workspace_bytes,
//                     (void*)(&alpha), in_desc, inPtr, (void*)(&beta),  out_desc, outPtr);

//   cudnnDestroyTensorDescriptor(in_desc);
//   cudnnDestroyTensorDescriptor(out_desc);
// }

template <>
void RowMax<float, lang::Cuda>(const Tensor* in, Tensor* out,
                               Context* ctx) {
  const float* inPtr = static_cast<const float*>(in->block()->data());
  float* outPtr = static_cast<float*>(out->block()->mutable_data());
  const size_t nrow = in->shape()[0];
  const size_t ncol = in->shape()[1];

  if(in->transpose()){
    Tensor t(in->shape(), in->device(), in->data_type());
    float* tPtr = static_cast<float*>(t.block()->mutable_data());
    float alpha[1] = {1.0};
    float beta[1] = {0.0};

    cudnnDataType_t cudnn_dtype = CUDNN_DATA_FLOAT;
    cudnnTensorDescriptor_t in_desc, t_desc;
    cudnnCreateTensorDescriptor(&in_desc);
    cudnnCreateTensorDescriptor(&t_desc);
    cudnnSetTensorNdDescriptor(in_desc, cudnn_dtype, in->generate_dim_cuda(), in->generate_shape_cuda().data(), in->generate_strides_cuda().data());
    cudnnSetTensorNdDescriptor(t_desc, cudnn_dtype, t.generate_dim_cuda(), t.generate_shape_cuda().data(), t.generate_strides_cuda().data());
    cudnnTransformTensor(ctx->cudnn_handle, (void*)(alpha), in_desc, inPtr,
                         (void*)(beta), t_desc, tPtr);

    const float* tPtr_const = static_cast<const float*>(t.block()->data());
    cuda::RowMax(nrow, ncol, tPtr_const, outPtr, ctx->stream);
    cudnnDestroyTensorDescriptor(in_desc);
    cudnnDestroyTensorDescriptor(t_desc);
  } else {
    cuda::RowMax(nrow, ncol, inPtr, outPtr, ctx->stream);
  }
}

}  // namespace singa

#endif  // USE_CUDA
#endif  // SINGA_CORE_TENSOR_TENSOR_MATH_CUDA_H_
