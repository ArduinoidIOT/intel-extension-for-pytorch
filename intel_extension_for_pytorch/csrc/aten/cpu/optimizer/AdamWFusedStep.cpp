#include "optimizer.h"

#include <torch/all.h>
#include <torch/csrc/autograd/function.h>

namespace torch_ipex {
namespace cpu {

DEFINE_DISPATCH(adamw_fused_step_kernel_stub);

void adamw_fused_step(
    const at::Tensor& param_,
    const at::Tensor& exp_avg_,
    const at::Tensor& exp_avg_sq_,
    const at::Tensor& max_exp_avg_sq_,
    const at::Tensor& grad_,
    const at::Tensor& param2_,
    bool amsgrad,
    double step,
    double beta1,
    double beta2,
    double learning_rate,
    double weight_decay,
    double eps) {
  RECORD_FUNCTION(
      "torch_ipex::adamw_fused_step", c10::ArrayRef<c10::IValue>({}));

  TORCH_CHECK(
      learning_rate >= 0, "Expect learning rate >= 0.0, got ", learning_rate);
  TORCH_CHECK(eps >= 0, "Expect eps >= 0.0, got ", eps);
  TORCH_CHECK(beta1 >= 0 && beta1 < 1, "Expect 0.0 <= beta1 < 1.0, got", beta1);
  TORCH_CHECK(beta2 >= 0 && beta2 < 1, "Expect 0.0 <= beta2 < 1.0, got", beta2);
  TORCH_CHECK(
      weight_decay >= 0, "Expect weight_decay >= 0.0, got ", weight_decay);

  TORCH_CHECK(
      param_.sizes() == grad_.sizes(),
      "Expect param and grad have the same sizes, param sizes: ",
      param_.sizes(),
      "; grad sizes: ",
      grad_.sizes());
  TORCH_CHECK(
      param_.sizes() == exp_avg_.sizes(),
      "Expect param and exp_avg have the same sizes, param sizes: ",
      param_.sizes(),
      "; exp_avg sizes: ",
      exp_avg_.sizes());
  TORCH_CHECK(
      param_.sizes() == exp_avg_sq_.sizes(),
      "Expect param and exp_avg_sq_ have the same sizes, param sizes: ",
      param_.sizes(),
      "; exp_avg_sq sizes: ",
      exp_avg_sq_.sizes());
  if (amsgrad) {
    TORCH_CHECK(
        param_.sizes() == max_exp_avg_sq_.sizes(),
        "Expect param and max_exp_avg_sq_ have the same sizes, param sizes: ",
        param_.sizes(),
        "; max_exp_avg_sq sizes: ",
        max_exp_avg_sq_.sizes());
  }
  TORCH_CHECK(
      param2_.numel() == 0 || param_.sizes() == param2_.sizes(),
      "Expect param and param2_ have the same sizes, param sizes: ",
      param_.sizes(),
      "; param2_ sizes: ",
      param2_.sizes());

  /*
  pointer to adamw_fused_step_kernel_impl(
      param_,
      exp_avg_,
      exp_avg_sq_,
      max_exp_avg_sq_,
      grad_,
      param2_,
      amsgrad,
      step,
      beta1,
      beta2,
      learning_rate,
      weight_decay,
      eps);
  */
  adamw_fused_step_kernel_stub(
      kCPU,
      param_,
      exp_avg_,
      exp_avg_sq_,
      max_exp_avg_sq_,
      grad_,
      param2_,
      amsgrad,
      step,
      beta1,
      beta2,
      learning_rate,
      weight_decay,
      eps);
}

} // namespace cpu
} // namespace torch_ipex

namespace {

TORCH_LIBRARY_FRAGMENT(torch_ipex, m) {
  m.def(
      "adamw_fused_step(Tensor(a!) param, Tensor(b!) exp_avg, Tensor(c!) "
      "exp_avg_sq, Tensor(d!) max_exp_avg_sq, Tensor grad, Tensor trail, "
      "bool amsgrad, float step, float beta1, float "
      "beta2, float lr, float weight_decay, float eps) -> ()",
      torch_ipex::cpu::adamw_fused_step);
}

} // namespace
