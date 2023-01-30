/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "oneflow/core/framework/op_expr_grad_function.h"
#include "oneflow/core/functional/functional.h"

namespace oneflow {
namespace one {

struct DependCaptureState : public AutoGradCaptureState {
  bool in_requires_grad = false;
  bool depend_tensor_requires_grad = false;
  Shape depend_tensor_shape;
};

class Depend : public OpExprGradFunction<DependCaptureState> {
 public:
  Maybe<void> Init(const OpExpr& op) override { return Maybe<void>::Ok(); }

  Maybe<void> Capture(DependCaptureState* ctx, const TensorTuple& inputs,
                      const TensorTuple& outputs, const AttrMap& attrs) const override {
    CHECK_EQ_OR_RETURN(inputs.size(), 2);   // NOLINT(maybe-need-error-msg)
    CHECK_EQ_OR_RETURN(outputs.size(), 1);  // NOLINT(maybe-need-error-msg)
    ctx->in_requires_grad = inputs.at(0)->requires_grad();
    ctx->depend_tensor_requires_grad = inputs.at(1)->requires_grad();
    if (ctx->depend_tensor_requires_grad) { ctx->depend_tensor_shape = *(inputs.at(1)->shape()); }
    return Maybe<void>::Ok();
  }

  Maybe<void> Apply(const DependCaptureState* ctx, const TensorTuple& out_grads,
                    TensorTuple* in_grads) const override {
    CHECK_EQ_OR_RETURN(out_grads.size(), 1);  // NOLINT(maybe-need-error-msg)
    in_grads->resize(2);
    if (ctx->in_requires_grad) { in_grads->at(0) = out_grads.at(0); }
    if (ctx->depend_tensor_requires_grad) {
      in_grads->at(1) =
          JUST(functional::Constant(ctx->depend_tensor_shape, Scalar(0), out_grads.at(0)->dtype(),
                                    JUST(out_grads.at(0)->device())));
    }
    return Maybe<void>::Ok();
  }
};

REGISTER_OP_EXPR_GRAD_FUNCTION("depend", Depend);

}  // namespace one
}  // namespace oneflow
