#include "Matmul.h"
#include <ATen/ATen.h>
#include <ATen/native/Resize.h>
#include <torch/all.h>
#include <torch/csrc/autograd/functions/utils.h>
#include "csrc/cpu/ideep/IDeepConversions.h"
#include "csrc/cpu/ideep/ideep.hpp"
#include "csrc/utils/fpmath_mode.h"
#include "csrc/utils/library.h"
namespace torch {
namespace autograd {
namespace generated {
namespace ipex {
void copy_range(variable_list& out, IndexRange range, const Tensor& t) {
  AT_ASSERT(range.second <= out.size());
  AT_ASSERTM(
      range.second - range.first == 1, "inconsistent range for Tensor output");
  out[range.first] = t;
}

bool any_variable_defined(const variable_list& variables) {
  for (const auto& variable : variables) {
    if (variable.defined()) {
      return true;
    }
  }
  return false;
}

variable_list BmmBackward0::apply(variable_list&& grads) {
  std::lock_guard<std::mutex> lock(mutex_);

  IndexRangeGenerator gen;
  auto self_ix = gen.range(1);
  auto mat2_ix = gen.range(1);
  variable_list grad_inputs(gen.size());
  const auto& grad = grads[0];
  auto self = self_.unpack();
  auto mat2 = mat2_.unpack();
  bool any_grad_defined = any_variable_defined(grads);
  if (should_compute_output({mat2_ix})) {
    at::Tensor grad_result = Tensor();
    if (any_grad_defined) {
      grad_result =
          torch_ipex::cpu::matmul_onednn(self.transpose(1, 2).conj(), grad);
    }
    copy_range(grad_inputs, mat2_ix, grad_result);
  }
  if (should_compute_output({self_ix})) {
    at::Tensor grad_result = Tensor();
    if (any_grad_defined) {
      grad_result =
          torch_ipex::cpu::matmul_onednn(grad, mat2.transpose(1, 2).conj());
    }
    copy_range(grad_inputs, self_ix, grad_result);
  }
  return grad_inputs;
}
} // namespace ipex
} // namespace generated
} // namespace autograd
} // namespace torch

namespace torch_ipex {
namespace cpu {

at::IntArrayRef strides_or_error(
    const at::Tensor& input,
    c10::string_view const& input_name) {
  // TODO: Ideally, this function would never be called if requires_grad is
  // not set. Once codegen is updated to avoid the call, we can remove this
  // check.
  if (input.requires_grad()) {
    TORCH_CHECK(
        !input.is_sparse(),
        "The backward pass for this operation requires the '",
        input_name,
        "' tensor to be strided, but a sparse tensor was given instead. ",
        "Please either use a strided tensor or set requires_grad=False for '",
        input_name,
        "'");
    return input.strides();
  } else {
    return at::IntArrayRef({});
  }
}

at::Tensor toNonOptTensor(const c10::optional<at::Tensor>& t) {
  return t.has_value() ? *t : at::Tensor();
}

at::Tensor toNonOptFwGrad(const c10::optional<at::Tensor>& t) {
  return (t.has_value() && t->defined()) ? t->_fw_grad(/*level */ 0)
                                         : at::Tensor();
}

at::Tensor toNonOptPrimal(const c10::optional<at::Tensor>& t) {
  return (t.has_value() && t->defined()) ? t->_fw_primal(/*level */ 0)
                                         : at::Tensor();
}

// The following code is copied from PyTorch generated code.
// generated by
// https://github.com/pytorch/pytorch/blob/master/tools/autograd/gen_variable_type.py
void handle_grad(
    const at::Tensor& self,
    const at::Tensor& mat2,
    at::Tensor& output) {
  if (torch::autograd::compute_requires_grad(self, mat2)) {
    auto dim_tensor1 = self.dim();
    auto dim_tensor2 = mat2.dim();
    if ((dim_tensor1 >= 1 && dim_tensor2 >= 1) &&
        (dim_tensor1 >= 3 || dim_tensor2 >= 3)) {
      std::shared_ptr<torch::autograd::generated::ipex::BmmBackward0> grad_fn =
          std::shared_ptr<torch::autograd::generated::ipex::BmmBackward0>(
              new torch::autograd::generated::ipex::BmmBackward0(),
              torch::autograd::deleteNode);
      grad_fn->set_next_edges(torch::autograd::collect_next_edges(self, mat2));
      if (grad_fn->should_compute_output(1)) {
        grad_fn->self_ = torch::autograd::SavedVariable(self, false);
      }
      if (grad_fn->should_compute_output(0)) {
        grad_fn->mat2_ = torch::autograd::SavedVariable(mat2, false);
      }
      torch::autograd::set_history(output, grad_fn);
    } else {
      std::shared_ptr<torch::autograd::generated::MmBackward0> grad_fn =
          std::shared_ptr<torch::autograd::generated::MmBackward0>(
              new torch::autograd::generated::MmBackward0(),
              torch::autograd::deleteNode);
      grad_fn->set_next_edges(torch::autograd::collect_next_edges(self, mat2));
      if (grad_fn->should_compute_output(1)) {
        grad_fn->self_ = torch::autograd::SavedVariable(self, false);
      }
      grad_fn->mat2_sizes = mat2.sizes().vec();
      grad_fn->mat2_strides = strides_or_error(mat2, "mat2").vec();
      grad_fn->mat2_layout = mat2.layout();
      grad_fn->self_sizes = self.sizes().vec();
      grad_fn->self_strides = strides_or_error(self, "self").vec();
      grad_fn->self_layout = self.layout();
      if (grad_fn->should_compute_output(0)) {
        grad_fn->mat2_ = torch::autograd::SavedVariable(mat2, false);
      }
      torch::autograd::set_history(output, grad_fn);
    }
  }

  if (torch::autograd::isFwGradDefined(self) ||
      torch::autograd::isFwGradDefined(mat2)) {
    auto self_t_raw = toNonOptFwGrad(self);
    auto self_tensor = toNonOptTensor(self);
    auto self_t = (self_t_raw.defined() || !self_tensor.defined())
        ? self_t_raw
        : at::_efficientzerotensor(self_tensor.sizes(), self_tensor.options());
    auto self_p = toNonOptPrimal(self);
    auto mat2_t_raw = toNonOptFwGrad(mat2);
    auto mat2_tensor = toNonOptTensor(mat2);
    auto mat2_t = (mat2_t_raw.defined() || !mat2_tensor.defined())
        ? mat2_t_raw
        : at::_efficientzerotensor(mat2_tensor.sizes(), mat2_tensor.options());
    auto mat2_p = toNonOptPrimal(mat2);
    c10::optional<at::Tensor> result_new_fw_grad_opt =
        self_t.matmul(mat2_p) + self_p.matmul(mat2_t);
    // The hardcoded 0 here will need to be updated once we support multiple
    // levels.
    output._set_fw_grad(
        result_new_fw_grad_opt.value(),
        /* level */ 0,
        /* is_inplace_op */ false);
  }
}

at::Tensor matmul_onednn(const at::Tensor& self, const at::Tensor& mat2) {
#if defined(IPEX_DISP_OP)
  printf("torch_ipex::matmul_onednn\n");
#endif

  RECORD_FUNCTION("torch_ipex::matmul_onednn", c10::ArrayRef<c10::IValue>({}));
  auto tensor1_ = self.is_contiguous() ? self : self.contiguous();
  auto tensor2_ = mat2.is_contiguous() ? mat2 : mat2.contiguous();
  const int64_t dim = self.dim();
  const ideep::tensor mkldnn_input = itensor_view_from_dense(tensor1_);
  const ideep::tensor mkldnn_tensor2 = itensor_view_from_dense(tensor2_);

  std::vector<int64_t> output_size(dim);
  for (auto i = 0; i < dim - 1; i++) {
    output_size[i] = self.size(i);
  }
  output_size[dim - 1] = mat2.size(dim - 1);
  auto output = at::empty(output_size, self.options());
  ideep::tensor mkldnn_output = itensor_view_from_dense(output);

  ideep::matmul_forward::compute(
      mkldnn_input,
      mkldnn_tensor2,
      mkldnn_output,
      1.0f,
      1.0f,
      ideep::scale_t(),
      ideep::scale_t(),
      ideep::scale_t(),
      ideep::attr_t());

  handle_grad(self, mat2, output);
  return output;
}

at::Tensor& matmul_onednn(
    at::Tensor& out,
    const at::Tensor& self,
    const at::Tensor& mat2) {
#if defined(IPEX_DISP_OP)
  printf("torch_ipex::matmul_onednn_out\n");
#endif

  RECORD_FUNCTION(
      "torch_ipex::matmul_onednn_out", c10::ArrayRef<c10::IValue>({}));
  auto tensor1_ = self.is_contiguous() ? self : self.contiguous();
  auto tensor2_ = mat2.is_contiguous() ? mat2 : mat2.contiguous();
  const int64_t dim = self.dim();
  const ideep::tensor mkldnn_input = itensor_view_from_dense(tensor1_);
  const ideep::tensor mkldnn_tensor2 = itensor_view_from_dense(tensor2_);

  std::vector<int64_t> output_size(dim);
  for (auto i = 0; i < dim - 1; i++) {
    output_size[i] = self.size(i);
  }
  output_size[dim - 1] = mat2.size(dim - 1);
  if (out.dim() != output_size.size() ||
      !std::equal(
          out.sizes().begin() + 1,
          out.sizes().end(),
          output_size.begin() + 1)) {
    at::native::resize_output(out, output_size);
  }
  ideep::tensor mkldnn_output = itensor_view_from_dense(out);

  ideep::matmul_forward::compute(
      mkldnn_input,
      mkldnn_tensor2,
      mkldnn_output,
      1.0f,
      1.0f,
      ideep::scale_t(),
      ideep::scale_t(),
      ideep::scale_t(),
      ideep::attr_t());
  return out;
}

/*
Matrix product of two Tensors.
The behavior depends on the dimensionality of the Tensors as follows:
- If both Tensors are 1-dimensional, the dot product (scalar) is returned.
- If both arguments are 2-dimensional, the matrix-matrix product is returned.
- If the first argument is 1-dimensional and the second argument is
2-dimensional, a 1 is prepended to its dimension for the purpose of the matrix
multiply. After the matrix multiply, the prepended dimension is removed.
- If the first argument is 2-dimensional and the second argument is
1-dimensional, the matrix-vector product is returned.
- If both arguments are at least 1-dimensional and at least one argument is
  N-dimensional (where N > 2), then a batched matrix multiply is returned.  If
the first argument is 1-dimensional, a 1 is prepended to its dimension for the
purpose of the batched matrix multiply and removed after.  If the second
argument is 1-dimensional, a 1 is appended to its dimension for the purpose of
the batched matrix multiple and removed after. The non-matrix (i.e. batch)
dimensions are broadcasted (and thus must be broadcastable).  For example, if
tensor1 is a (j x 1 x n x m) Tensor and tensor2 is a (k x m x p) Tensor, the
returned tensor will be an (j x k x n x p) Tensor.
*/
at::Tensor matmul_impl(
    c10::optional<at::Tensor> out_opt,
    const at::Tensor& tensor1,
    const at::Tensor& tensor2) {
  auto has_out = out_opt.has_value();
  at::Tensor out = out_opt.value_or(at::Tensor());
  bool useOneDNN = tensor1.device().is_cpu() && tensor2.device().is_cpu() &&
      (!out.defined() || out.device().is_cpu()) &&
      (torch_ipex::getFP32MathModeCpu() == torch_ipex::FP32MathMode::BF32) &&
      (tensor1.scalar_type() == at::ScalarType::Float) &&
      (tensor2.scalar_type() == at::ScalarType::Float) &&
      (tensor1.numel() > 0 && tensor2.numel() > 0);
  at::NoNamesGuard guard;
  auto dim_tensor1 = tensor1.dim();
  auto dim_tensor2 = tensor2.dim();

  if (dim_tensor1 == 1 && dim_tensor2 == 1) {
    return has_out ? at::native::dot_out(tensor1, tensor2, out)
                   : tensor1.dot(tensor2);
  } else if (dim_tensor1 == 2 && dim_tensor2 == 1) {
    return has_out ? at::mv_out(out, tensor1, tensor2) : tensor1.mv(tensor2);
  } else if (dim_tensor1 == 1 && dim_tensor2 == 2) {
    if (useOneDNN) {
      return has_out
          ? matmul_onednn(out, tensor1.unsqueeze(0), tensor2).squeeze_(0)
          : matmul_onednn(tensor1.unsqueeze(0), tensor2).squeeze_(0);
    } else {
      return has_out
          ? at::mm_out(out, tensor1.unsqueeze(0), tensor2).squeeze_(0)
          : tensor1.unsqueeze(0).mm(tensor2).squeeze_(0);
    }
  } else if (dim_tensor1 == 2 && dim_tensor2 == 2) {
    if (useOneDNN) {
      return has_out ? matmul_onednn(out, tensor1, tensor2)
                     : matmul_onednn(tensor1, tensor2);
    } else {
      return has_out ? at::mm_out(out, tensor1, tensor2) : tensor1.mm(tensor2);
    }
  } else if (dim_tensor1 >= 3 && (dim_tensor2 == 1 || dim_tensor2 == 2)) {
    // optimization: use mm instead of bmm by folding tensor1's batch into
    // its leading matrix dimension.

    at::Tensor t2 = dim_tensor2 == 1 ? tensor2.unsqueeze(-1) : tensor2;
    auto size1 = tensor1.sizes();
    auto size2 = t2.sizes();
    std::vector<int64_t> output_size;
    output_size.insert(output_size.end(), size1.begin(), size1.end() - 1);
    if (dim_tensor2 > 1) {
      output_size.push_back(size2[dim_tensor2 - 1]);
    }

    // fold the batch into the first dimension
    // Why not tensor1.view(-1, size1[size1.size() -1])?
    // If the last dim is 0, then view(-1, 0) won't work because the -1 becomes
    // ambiguous. This can happen in e.g. [3, 5, 0] @ [0, 0]. So we manually
    // compute the folding as a result.
    const auto dim1_size =
        c10::multiply_integers(size1.begin(), size1.end() - 1);
    auto t1 =
        tensor1.expect_contiguous()->view({dim1_size, size1[size1.size() - 1]});
    if (useOneDNN) {
      at::Tensor output = has_out
          ? at::_unsafe_view(matmul_onednn(out, t1, t2), output_size)
          : at::_unsafe_view(matmul_onednn(t1, t2), output_size);
      return has_out ? out.set_(output) : output;
    } else {
      at::Tensor output = has_out
          ? at::_unsafe_view(at::mm_out(out, t1, t2), output_size)
          : at::_unsafe_view(t1.mm(t2), output_size);
      return has_out ? out.set_(output) : output;
    }
  } else if ((dim_tensor1 == 1 || dim_tensor1 == 2) && dim_tensor2 >= 3) {
    // optimization: transpose the inner dimensions of the arguments, call
    // matmul on the swapped arguments, then transpose the inner dimensions
    // of the result.
    const int64_t n = dim_tensor1 == 2 ? tensor1.size(-2) : 1;
    const int64_t m = tensor1.size(-1);
    const int64_t p = tensor2.size(-1);

    const at::Tensor t2_T = tensor2.transpose(-1, -2);
    const at::Tensor t1_T =
        dim_tensor1 == 2 ? tensor1.t() : tensor1.reshape({n, m}).t();
    const at::Tensor res_T = matmul_impl(out_opt, t2_T, t1_T);

    if (dim_tensor1 == 2) {
      at::Tensor res = res_T.transpose(-1, -2).contiguous();
      return has_out ? out.set_(res) : res;
    } else {
      std::vector<int64_t> shape =
          tensor2.sizes().slice(0, dim_tensor2 - 2).vec();
      shape.push_back(p);

      at::Tensor res = res_T.reshape(shape).contiguous();
      return has_out ? out.set_(res) : res;
    }
  } else if (
      (dim_tensor1 >= 1 && dim_tensor2 >= 1) &&
      (dim_tensor1 >= 3 || dim_tensor2 >= 3)) {
    // We are multiplying b1 x n x m1 by x2 x m2 x p (where b1 can be a list);
    // we track m1 vs m2 separately even though they must match for nicer error
    // messages
    int64_t n = dim_tensor1 > 1 ? tensor1.size(-2) : 1;
    int64_t m1 = tensor1.size(-1);
    c10::IntArrayRef batch_tensor1(
        tensor1.sizes().data(), std::max<int64_t>(dim_tensor1 - 2, 0));
    int64_t m2 = dim_tensor2 > 1 ? tensor2.size(-2) : 1;
    int64_t p = tensor2.size(-1);
    c10::IntArrayRef batch_tensor2(
        tensor2.sizes().data(), std::max<int64_t>(dim_tensor2 - 2, 0));

    // expand the batch portion (i.e. cut off matrix dimensions and expand rest)
    std::vector<int64_t> expand_batch_portion =
        at::infer_size(batch_tensor1, batch_tensor2);

    std::vector<int64_t> tensor1_expand_size(expand_batch_portion);
    tensor1_expand_size.insert(tensor1_expand_size.end(), {n, m1});

    std::vector<int64_t> tensor2_expand_size(expand_batch_portion);
    tensor2_expand_size.insert(tensor2_expand_size.end(), {m2, p});

    const int64_t expand_batch_product =
        c10::multiply_integers(expand_batch_portion);

    std::vector<int64_t> tensor1_bmm_view({expand_batch_product});
    tensor1_bmm_view.insert(tensor1_bmm_view.end(), {n, m1});

    std::vector<int64_t> tensor2_bmm_view({expand_batch_product});
    tensor2_bmm_view.insert(tensor2_bmm_view.end(), {m2, p});

    // flatten expanded batches
    at::Tensor tensor1_expanded =
        tensor1.expand(tensor1_expand_size).reshape(tensor1_bmm_view);
    at::Tensor tensor2_expanded =
        tensor2.expand(tensor2_expand_size).reshape(tensor2_bmm_view);

    // reshape batches back into result
    std::vector<int64_t> output_shape(expand_batch_portion);
    if (dim_tensor1 > 1) {
      output_shape.push_back(n);
    }
    if (dim_tensor2 > 1) {
      output_shape.push_back(p);
    }
    if (useOneDNN) {
      at::Tensor output = has_out
          ? at::_unsafe_view(
                matmul_onednn(out, tensor1_expanded, tensor2_expanded),
                output_shape)
          : at::_unsafe_view(
                matmul_onednn(tensor1_expanded, tensor2_expanded),
                output_shape);

      return has_out ? out.set_(output) : output;
    } else {
      at::Tensor output = has_out
          ? at::_unsafe_view(
                at::bmm_out(out, tensor1_expanded, tensor2_expanded),
                output_shape)
          : at::_unsafe_view(
                tensor1_expanded.bmm(tensor2_expanded), output_shape);

      return has_out ? out.set_(output) : output;
    }
  }

  AT_ERROR(
      "both arguments to matmul need to be at least 1D, but they are ",
      dim_tensor1,
      "D and ",
      dim_tensor2,
      "D");
}

at::Tensor matmul_cpu(const at::Tensor& tensor1, const at::Tensor& tensor2) {
  auto maybe_outnames =
      at::namedinference::compute_matmul_outnames(tensor1, tensor2);
  auto result = matmul_impl(c10::nullopt, tensor1, tensor2);
  at::namedinference::propagate_names_if_nonempty(result, maybe_outnames);
  return result;
}

at::Tensor& matmul_out_cpu(
    const at::Tensor& tensor1,
    const at::Tensor& tensor2,
    at::Tensor& result) {
  auto maybe_outnames =
      at::namedinference::compute_matmul_outnames(tensor1, tensor2);
  matmul_impl(c10::optional<at::Tensor>(result), tensor1, tensor2);
  at::namedinference::propagate_names_if_nonempty(result, maybe_outnames);
  return result;
}

} // namespace cpu
} // namespace torch_ipex

namespace {

IPEX_TORCH_LIBRARY_IMPL(aten, CompositeImplicitAutograd, m) {
  m.impl(
      TORCH_SELECTIVE_NAME("aten::matmul"),
      TORCH_FN((&torch_ipex::cpu::matmul_cpu)));
  m.impl(
      TORCH_SELECTIVE_NAME("aten::matmul.out"),
      TORCH_FN((&torch_ipex::cpu::matmul_out_cpu)));
}

} // namespace
