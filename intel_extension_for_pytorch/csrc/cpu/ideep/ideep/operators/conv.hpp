#ifndef IDEEP_OPERATORS_CONV_HPP
#define IDEEP_OPERATORS_CONV_HPP

namespace ideep {

struct convolution_forward_params {
  dnnl::convolution_forward::primitive_desc pd;
  // bias_attr contains requantization scales for bias
  attr_t bias_attr;
  scale_t dst_scales;
  int groups;
  attr_t op_attr;
  int pd_use_threads;
};

struct convolution_forward
    : public dnnl::convolution_forward,
      utils::computation_cache<dnnl::convolution_forward::primitive_desc> {
  using super = dnnl::convolution_forward;

  // prepare with bias
  static void prepare(
      convolution_forward_params& param,
      const tensor& src,
      const tensor& weights,
      const tensor& bias,
      const dims& dst_dims,
      tensor& dst,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      int groups,
      const scale_t& src_scales = scale_t(),
      const scale_t& weights_scales = scale_t(),
      const scale_t& dst_scales = scale_t(),
      const attr_t& attr = attr_t(),
      algorithm aalgorithm = algorithm::convolution_direct,
      prop_kind aprop_kind = prop_kind::forward,
      const lowp_kind alowp_kind = u8s8,
      const engine& aengine = engine::cpu_engine()) {
    do_prepare</*with_bias=*/true, /*keep_format=*/false>(
        param,
        src,
        weights,
        bias,
        dst_dims,
        dst,
        strides,
        dilates,
        padding_l,
        padding_r,
        groups,
        src_scales,
        weights_scales,
        dst_scales,
        attr,
        aalgorithm,
        aprop_kind,
        alowp_kind,
        aengine);
  }

  // prepare without bias
  static void prepare(
      convolution_forward_params& param,
      const tensor& src,
      const tensor& weights,
      const dims& dst_dims,
      tensor& dst,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      int groups,
      const scale_t& src_scales = scale_t(),
      const scale_t& weights_scales = scale_t(),
      const scale_t& dst_scales = scale_t(),
      const attr_t& attr = attr_t(),
      algorithm aalgorithm = algorithm::convolution_direct,
      prop_kind aprop_kind = prop_kind::forward,
      const lowp_kind alowp_kind = u8s8,
      const engine& aengine = engine::cpu_engine()) {
    static tensor dummy_bias;
    do_prepare</*with_bias=*/false, /*keep_format=*/false>(
        param,
        src,
        weights,
        dummy_bias,
        dst_dims,
        dst,
        strides,
        dilates,
        padding_l,
        padding_r,
        groups,
        src_scales,
        weights_scales,
        dst_scales,
        attr,
        aalgorithm,
        aprop_kind,
        alowp_kind,
        aengine);
  }

  // compute with bias
  static void compute(
      const convolution_forward_params& param,
      const dnnl::convolution_forward& prim,
      const tensor& src,
      const tensor& weights,
      const tensor& bias,
      tensor& dst) {
    do_compute</*with_bias=*/true>(param, prim, src, weights, bias, dst);
  }

  // compute without bias
  static void compute(
      const convolution_forward_params& param,
      const dnnl::convolution_forward& prim,
      const tensor& src,
      const tensor& weights,
      tensor& dst) {
    static tensor dummy_bias;
    do_compute</*with_bias=*/false>(param, prim, src, weights, dummy_bias, dst);
  }

  // 2-in-1 compute (prepare & compute) with bias
  template <bool plain_format = false>
  static void compute(
      const tensor& src,
      const tensor& weights,
      const tensor& bias,
      const dims& dst_dims,
      tensor& dst,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      int groups,
      const scale_t& src_scales = scale_t(),
      const scale_t& weights_scales = scale_t(),
      const scale_t& dst_scales = scale_t(),
      const attr_t& attr = attr_t(),
      algorithm aalgorithm = algorithm::convolution_direct,
      prop_kind aprop_kind = prop_kind::forward,
      const lowp_kind alowp_kind = u8s8,
      const engine& aengine = engine::cpu_engine()) {
    compute_dispatch</*with_bias=*/true, plain_format>(
        src,
        weights,
        bias,
        dst_dims,
        dst,
        strides,
        dilates,
        padding_l,
        padding_r,
        groups,
        src_scales,
        weights_scales,
        dst_scales,
        attr,
        aalgorithm,
        aprop_kind,
        alowp_kind,
        aengine);
  }

  // 2-in-1 compute (prepare & compute) without bias
  template <bool plain_format = false>
  static void compute(
      const tensor& src,
      const tensor& weights,
      const dims& dst_dims,
      tensor& dst,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      int groups,
      const scale_t& src_scales = scale_t(),
      const scale_t& weights_scales = scale_t(),
      const scale_t& dst_scales = scale_t(),
      const attr_t& attr = attr_t(),
      algorithm aalgorithm = algorithm::convolution_direct,
      prop_kind aprop_kind = prop_kind::forward,
      const lowp_kind alowp_kind = u8s8,
      const engine& aengine = engine::cpu_engine()) {
    static tensor dummy_bias;
    compute_dispatch</*with_bias=*/false, plain_format>(
        src,
        weights,
        dummy_bias,
        dst_dims,
        dst,
        strides,
        dilates,
        padding_l,
        padding_r,
        groups,
        src_scales,
        weights_scales,
        dst_scales,
        attr,
        aalgorithm,
        aprop_kind,
        alowp_kind,
        aengine);
  }

  template <bool channels_last = false>
  static tensor::desc expected_weights_desc(
      const dims& weights_dims,
      data_type dtype = data_type::f32,
      const dims& strides = {1, 1},
      const dims& padding_l = {0, 0},
      const dims& padding_r = {0, 0},
      const dims& dilates = {1, 1},
      int groups = 1,
      algorithm aalgorithm = algorithm::convolution_direct,
      prop_kind aprop_kind = prop_kind::forward,
      data_type x_dtype = data_type::f32,
      const dims& src_dims = dims(),
      const attr_t& attr = attr_t(),
      const engine& aengine = engine::cpu_engine()) {
    auto src_size = weights_dims.size(); // weights_dims is 3 for conv1d, 4 for
                                         // conv2d and 5 for conv3d
    auto grouped = groups > 1;
    auto weights_dims_g =
        grouped ? utils::group_dims(weights_dims, groups) : weights_dims;
    auto weights_desc = tensor::desc(weights_dims_g, dtype);

    auto dims_in = weights_desc.get_dims();
    auto ndims = dims_in.size();
    auto dilates_ = utils::get_compatible_dilates(dilates, src_size);

    IDEEP_ENFORCE(
        !(aalgorithm == algorithm::convolution_winograd && src_dims.empty()),
        "Incorrect src_dims");
    dims x_dims, y_dims, kernel_size;
    auto ic = groups * dims_in[1 + grouped];
    auto oc = groups * dims_in[0 + grouped];
    if (5 == src_size) {
      kernel_size.push_back(dims_in[ndims - 3]);
      kernel_size.push_back(dims_in[ndims - 2]);
    }
    if (4 == src_size) {
      kernel_size.push_back(dims_in[ndims - 2]);
    }
    kernel_size.push_back(dims_in[ndims - 1]);
    if (src_dims.empty()) {
      // Construct a dummy case, those shapes are from resnet50 model,
      // just make some simple tests to make sure it can get our expected
      // format(real case), may be changed in the future.
      x_dims.push_back(32);
      x_dims.push_back(ic);
      y_dims.push_back(32);
      y_dims.push_back(oc);
      x_dims.push_back(14 * kernel_size[0]);
      if (4 == src_size) {
        x_dims.push_back(14 * kernel_size[1]);
      } else if (5 == src_size) {
        x_dims.push_back(14 * kernel_size[1]);
        x_dims.push_back(14 * kernel_size[2]);
      }
    } else {
      // Use the real data
      for (auto i = 0; i < src_size; ++i) {
        x_dims.push_back(src_dims[i]);
      }
      y_dims.push_back(src_dims[0]);
      y_dims.push_back(oc);
    }
    for (auto d = 2; d < src_size; ++d) {
      auto out_size =
          (x_dims[d] - ((kernel_size[d - 2] - 1) * (dilates_[d - 2] + 1) + 1) +
           (padding_l[d - 2] + padding_r[d - 2])) /
              strides[d - 2] +
          1;
      y_dims.push_back(out_size);
    }
    x_dtype = dtype == data_type::bf16 ? dtype : x_dtype;
    auto y_dtype = dtype != data_type::s8 ? dtype : data_type::s32;
    tensor::desc src_desc(x_dims, x_dtype);
    tensor::desc dst_desc(y_dims, y_dtype);
    auto src_query = src_desc;
    auto dst_query = dst_desc;
    if (channels_last) {
      if (4 == src_size) {
        src_query = src_desc.to_format(tag::nhwc);
        dst_query = dst_desc.to_format(tag::nhwc);
      } else if (5 == src_size) {
        src_query = src_desc.to_format(tag::ndhwc);
        dst_query = dst_desc.to_format(tag::ndhwc);
      }
    }
    if (3 == src_size) {
      src_query = src_desc.to_format(tag::nwc);
      dst_query = dst_desc.to_format(tag::nwc);
    }

    // FIXME: workaroud winograd format issue in inference
    // If prop_kind == forward_inference, the dnnl_wino_fmt for weights is
    // required by winograd primitive. Then, in the cases of variable input
    // shape, the detials of dnnl_wino_fmt will be changed. And, extra weihgts
    // reorder is inevitable each time, leading to bad performance. Here, we set
    // the prop_kind to forward, in order to reorder and cache weights as
    // blocked format, instead of dnnl_wino_fmt.
    auto apkind = aprop_kind;
    if (aalgorithm == algorithm::convolution_winograd &&
        aprop_kind == prop_kind::forward_inference) {
      apkind = prop_kind::forward;
    }
    attr_t op_attr = attr;
    op_attr.set_scratchpad_mode(dnnl::scratchpad_mode::user);
    op_attr.set_fpmath_mode();
    auto pd = get_primitive_desc</*with_bias=*/false>(
        src_query,
        weights_desc,
        tensor::desc(),
        dst_query,
        strides,
        dilates_,
        padding_l,
        padding_r,
        op_attr,
        aalgorithm,
        apkind);

    // embed group info into weights_desc
    return tensor::desc(pd.weights_desc(), groups);
  }

  // [keep_format]
  // - Set to true would let onednn to choose the optimal
  //   blocked format for dst tensor
  // - Set to false would keep dst tensor format as it is.
  //   We used this mode in pytorch plain-in-plain-out path to force
  //   the dst to be plain as src, so that it would also instruct onednn
  //   to use gemm-based conv implementation. Apply to both NCHW and NHWC.
  template <bool with_bias, bool keep_format = false>
  static primitive_desc get_primitive_desc(
      const tensor::desc& src_desc,
      const tensor::desc& weights_desc,
      const tensor::desc& bias_desc,
      const tensor::desc& dst_desc,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      const attr_t& attr = attr_t(),
      algorithm aalgorithm = algorithm::convolution_direct,
      prop_kind aprop_kind = prop_kind::forward,
      const engine& aengine = engine::cpu_engine()) {
    auto src_desc_query = src_desc;
    auto weights_desc_query = weights_desc;
    auto bias_desc_query = with_bias ? bias_desc : tensor::desc();
    auto dst_desc_query = dst_desc;
    auto src_is_channels_last = src_desc.is_channels_last();
    if (!keep_format) {
      src_desc_query = src_desc.to_format_any();
      weights_desc_query = weights_desc.to_format_any();
      bias_desc_query = with_bias ? bias_desc.to_format_any() : tensor::desc();
      dst_desc_query = dst_desc.to_format_any();
    }

    // For nhwc / ndhwc path, weight uses format_tag::any,
    // while activation uses format_tag::nhwc / format_tag::ndhwc.
    bool channels_last =
        src_is_channels_last || weights_desc.is_channels_last();
    if (channels_last) {
      const auto dim = src_desc.get_ndims();
      auto memory_format = tag::nhwc;
      if (dim == 3) {
        memory_format = tag::nwc;
      } else if (dim == 5) {
        memory_format = tag::ndhwc;
      }
      src_desc_query = src_desc.to_format(memory_format);
      weights_desc_query = weights_desc.to_format_any();
      bias_desc_query = with_bias ? bias_desc.to_format_any() : tensor::desc();
      dst_desc_query = dst_desc.to_format(memory_format);
    }

    auto key = utils::create_key(
        aprop_kind,
        aalgorithm,
        src_desc_query,
        weights_desc_query,
        with_bias,
        strides,
        dilates,
        padding_l,
        padding_r,
        attr,
        omp_get_max_threads());
    return fetch_or_create(key, [&]() {
      if (with_bias) {
        return primitive_desc(
            {aprop_kind,
             aalgorithm,
             src_desc_query,
             weights_desc_query,
             bias_desc_query,
             dst_desc_query,
             strides,
             dilates,
             padding_l,
             padding_r},
            attr,
            aengine);
      } else {
        return primitive_desc(
            {aprop_kind,
             aalgorithm,
             src_desc_query,
             weights_desc_query,
             dst_desc_query,
             strides,
             dilates,
             padding_l,
             padding_r},
            attr,
            aengine);
      }
    });
  }

 private:
  static bool use_gemm(
      const dims& src,
      const dims& weight,
      const dims& dst,
      int groups) {
    if (groups != 1)
      return false;

    auto product = [](const dims& v, size_t start_offset = 0) {
      return std::accumulate(
          v.begin() + start_offset, v.end(), 1, std::multiplies<size_t>());
    };

    auto ker_spatial = product(weight, 2);
    bool pointwise = ker_spatial == 1;
    if (pointwise)
      return true;

    auto im2col_cost = ker_spatial * product(src);
    auto reorder_cost = product(src) + 2 * product(weight) + 2 * product(dst);
    return im2col_cost < reorder_cost;
  }

  template <bool with_bias, bool plain_format>
  static void compute_dispatch(
      const tensor& src,
      const tensor& weights,
      const tensor& bias,
      const dims& dst_dims,
      tensor& dst,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      int groups,
      const scale_t& src_scales = scale_t(),
      const scale_t& weights_scales = scale_t(),
      const scale_t& dst_scales = scale_t(),
      const attr_t& attr = attr_t(),
      algorithm aalgorithm = algorithm::convolution_direct,
      prop_kind aprop_kind = prop_kind::forward,
      const lowp_kind alowp_kind = u8s8,
      const engine& aengine = engine::cpu_engine()) {
    convolution_forward_params params;

    if (plain_format) {
      // Used for pytorch default CPU path, i.e. plain-in-plain-out
      // see [keep_format] for more details
      bool channels_last = src.get_desc().is_channels_last() ||
          weights.get_desc().is_channels_last();
      bool use_plain_dst =
          use_gemm(src.get_dims(), weights.get_dims(), dst_dims, groups) ||
          channels_last;
      if (use_plain_dst) {
        do_prepare<with_bias, /*keep_format=*/true>(
            params,
            src,
            weights,
            bias,
            dst_dims,
            dst,
            strides,
            dilates,
            padding_l,
            padding_r,
            groups,
            src_scales,
            weights_scales,
            dst_scales,
            attr,
            aalgorithm,
            aprop_kind,
            alowp_kind,
            aengine);
        do_compute<with_bias>(params, src, weights, bias, dst);
      } else {
        tensor dst_blocked;
        do_prepare<with_bias, /*keep_format=*/false>(
            params,
            src,
            weights,
            bias,
            dst_dims,
            dst_blocked,
            strides,
            dilates,
            padding_l,
            padding_r,
            groups,
            src_scales,
            weights_scales,
            dst_scales,
            attr,
            aalgorithm,
            aprop_kind,
            alowp_kind,
            aengine);
        do_compute<with_bias>(params, src, weights, bias, dst_blocked);
        dst.feed_from(dst_blocked);
      }
    } else {
      // Used for to_mkldnn() path
      do_prepare<with_bias, /*keep_format=*/false>(
          params,
          src,
          weights,
          bias,
          dst_dims,
          dst,
          strides,
          dilates,
          padding_l,
          padding_r,
          groups,
          src_scales,
          weights_scales,
          dst_scales,
          attr,
          aalgorithm,
          aprop_kind,
          alowp_kind,
          aengine);
      do_compute<with_bias>(params, src, weights, bias, dst);
    }
  }

  template <bool with_bias, bool keep_format>
  static void do_prepare(
      convolution_forward_params& param,
      const tensor& src,
      const tensor& weights,
      const tensor& bias,
      const dims& dst_dims,
      tensor& dst,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      int groups,
      const scale_t& src_scales,
      const scale_t& weights_scales,
      const scale_t& dst_scales,
      const attr_t& attr,
      algorithm aalgorithm,
      prop_kind aprop_kind,
      const lowp_kind alowp_kind,
      const engine& aengine) {
    scale_t dst_scales_in;
    data_type dst_data_type;
    tensor::desc src_desc, weights_desc, bias_desc;
    attr_t op_attr, src_attr, weights_attr, bias_attr;

    // make weights and dilates compatible with DNNL
    auto weights_ = weights.make_grouped_weights(groups);
    auto dilates_ = utils::get_compatible_dilates(dilates);

    op_attr = attr;

    IDEEP_ENFORCE(
        utils::one_of(
            weights_.get_data_type(), data_type::f32, data_type::bf16),
        "Incorrect data type in weights");

    // align weights data type with src
    dst_data_type = src.get_data_type() == data_type::bf16 ? data_type::bf16
                                                           : data_type::f32;
    src_desc = src.get_desc().to_type(dst_data_type);
    weights_desc = weights_.get_desc().to_type(dst_data_type);

    if (with_bias) {
      IDEEP_ENFORCE(
          utils::one_of(bias.get_data_type(), data_type::f32, data_type::bf16),
          "Incorrect data type in bias");
      bias_desc = bias.get_desc();
    }

    op_attr.set_scratchpad_mode(dnnl::scratchpad_mode::user);
    op_attr.set_fpmath_mode();
    auto dst_desc = attr.has_op_kind(kind::sum)
        ? dst.get_desc()
        : tensor::desc(dst_dims, dst_data_type);

    auto pd = get_primitive_desc<with_bias, keep_format>(
        src_desc,
        weights_desc,
        bias_desc,
        dst_desc,
        strides,
        dilates_,
        padding_l,
        padding_r,
        op_attr,
        aalgorithm,
        aprop_kind,
        aengine);

    param = {pd, bias_attr, dst_scales, groups, op_attr, omp_get_max_threads()};
  }

  template <bool with_bias>
  static void do_compute(
      const convolution_forward_params& param,
      const tensor& src,
      const tensor& weights,
      const tensor& bias,
      tensor& dst) {
    auto& pd = param.pd;
    // allocate scratchpad
    tensor scratchpad(pd.scratchpad_desc());
    auto expected_src = src.reorder_if_differ_in(pd.src_desc());
    tensor expected_weights;
    // it will be removed after block format reorder performance improved.
    if (!weights.get_desc().is_plain() &&
        weights.get_desc() != pd.weights_desc()) {
      expected_weights = weights.reorder_if_differ_in(pd.weights_desc());
    } else {
      expected_weights = weights.make_grouped_weights(param.groups)
                             .reorder_if_differ_in(pd.weights_desc());
    }

    auto expected_dst_desc = pd.dst_desc();
    tensor expected_dst;
    // dst not init in FW or has same desc with expected desc.
    if (dst.is_empty() || dst.get_desc() == expected_dst_desc) {
      dst.reinit_if_possible(expected_dst_desc);
      expected_dst = dst;
    } else {
      expected_dst.init(expected_dst_desc);
    }

    if (with_bias) {
      auto expected_bias =
          bias.reorder_if_differ_in(pd.bias_desc(), param.bias_attr);
      super(pd).execute(
          stream::default_stream(),
          {{DNNL_ARG_SRC, expected_src},
           {DNNL_ARG_WEIGHTS, expected_weights},
           {DNNL_ARG_BIAS, expected_bias},
           {DNNL_ARG_DST, expected_dst},
           {DNNL_ARG_SCRATCHPAD, scratchpad}});
    } else {
      super(pd).execute(
          stream::default_stream(),
          {{DNNL_ARG_SRC, expected_src},
           {DNNL_ARG_WEIGHTS, expected_weights},
           {DNNL_ARG_DST, expected_dst},
           {DNNL_ARG_SCRATCHPAD, scratchpad}});
    }

    // dst has been init in FW side, but has diff desc with expected_dst.
    if (dst.get_desc() != expected_dst.get_desc()) {
      dst.feed_from(expected_dst);
    }
  }

  template <bool with_bias>
  static void do_compute(
      const convolution_forward_params& param,
      const dnnl::convolution_forward& prim,
      const tensor& src,
      const tensor& weights,
      const tensor& bias,
      tensor& dst) {
    auto& pd = param.pd;
    // allocate scratchpad
    tensor scratchpad(pd.scratchpad_desc());
    auto expected_src = src.reorder_if_differ_in(pd.src_desc());
    tensor expected_weights;
    // it will be removed after block format reorder performance improved.
    if (!weights.get_desc().is_plain() &&
        weights.get_desc() != pd.weights_desc()) {
      expected_weights = weights.reorder_if_differ_in(pd.weights_desc());
    } else {
      expected_weights = weights.make_grouped_weights(param.groups)
                             .reorder_if_differ_in(pd.weights_desc());
    }

    auto expected_dst_desc = pd.dst_desc();
    tensor expected_dst;
    // dst not init in FW or has same desc with expected desc.
    if (dst.is_empty() || dst.get_desc() == expected_dst_desc) {
      dst.reinit_if_possible(expected_dst_desc);
      expected_dst = dst;
    } else {
      expected_dst.init(expected_dst_desc);
      if (param.op_attr.has_op_kind(kind::sum)) {
        expected_dst.feed_from(dst);
      }
    }

    if (with_bias) {
      auto expected_bias =
          bias.reorder_if_differ_in(pd.bias_desc(), param.bias_attr);
      prim.execute(
          stream::default_stream(),
          {{DNNL_ARG_SRC, expected_src},
           {DNNL_ARG_WEIGHTS, expected_weights},
           {DNNL_ARG_BIAS, expected_bias},
           {DNNL_ARG_DST, expected_dst},
           {DNNL_ARG_SCRATCHPAD, scratchpad}});
    } else {
      prim.execute(
          stream::default_stream(),
          {{DNNL_ARG_SRC, expected_src},
           {DNNL_ARG_WEIGHTS, expected_weights},
           {DNNL_ARG_DST, expected_dst},
           {DNNL_ARG_SCRATCHPAD, scratchpad}});
    }

    // dst has been init in FW side, but has diff desc with expected_dst.
    if (dst.get_desc() != expected_dst.get_desc()) {
      dst.feed_from(expected_dst);
    }
  }
};

struct convolution_backward_data : public dnnl::convolution_backward_data {
  using super = dnnl::convolution_backward_data;

  static void compute(
      const tensor& diff_dst,
      const tensor& weights,
      const dims& diff_src_dims,
      tensor& diff_src,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      const int groups,
      algorithm aalgorithm = algorithm::convolution_direct,
      const engine& aengine = engine::cpu_engine()) {
    // make weights and dilates compatible with DNNL
    auto weights_ = weights.make_grouped_weights(groups);
    auto dilates_ = utils::get_compatible_dilates(dilates);

    bool channels_last = diff_dst.get_desc().is_channels_last();
    auto format_tag = channels_last
        ? (diff_src_dims.size() == 4 ? tag::nhwc : tag::ndhwc)
        : tag::any;
    auto diff_dst_desc = diff_dst.get_desc().to_format(format_tag);
    // align weight data type with diff_dst for bf16
    auto weights_desc =
        weights_.get_desc().to_format_any().to_type(diff_dst.get_data_type());

    auto diff_src_desc =
        tensor::desc(diff_src_dims, diff_dst_desc.get_data_type(), format_tag);

    auto forward_hints =
        convolution_forward::get_primitive_desc</*with_bias=*/false>(
            diff_src_desc,
            weights_desc,
            tensor::desc(),
            diff_dst_desc,
            strides,
            dilates_,
            padding_l,
            padding_r);

    auto op_attr = ideep::attr_t();
    op_attr.set_fpmath_mode();
    op_attr.set_scratchpad_mode(dnnl::scratchpad_mode::user);

    auto pd = primitive_desc(
        {aalgorithm,
         diff_src_desc,
         weights_desc,
         diff_dst_desc,
         strides,
         dilates_,
         padding_l,
         padding_r},
        op_attr,
        aengine,
        forward_hints);

    auto expected_diff_dst = diff_dst.reorder_if_differ_in(pd.diff_dst_desc());
    auto expected_weights = weights_.reorder_if_differ_in(pd.weights_desc());

    auto expected_diff_src_desc = pd.diff_src_desc();
    tensor expected_diff_src;
    // diff_src not init in FW or has same desc with expected desc.
    if (diff_src.is_empty() || diff_src.get_desc() == expected_diff_src_desc) {
      diff_src.reinit_if_possible(expected_diff_src_desc);
      expected_diff_src = diff_src;
    } else {
      expected_diff_src.init(expected_diff_src_desc);
    }

    tensor scratchpad(pd.scratchpad_desc());

    super(pd).execute(
        stream::default_stream(),
        {{DNNL_ARG_DIFF_DST, expected_diff_dst},
         {DNNL_ARG_WEIGHTS, expected_weights},
         {DNNL_ARG_DIFF_SRC, expected_diff_src},
         {DNNL_ARG_SCRATCHPAD, scratchpad}});
  }
};

struct convolution_backward_weights
    : public dnnl::convolution_backward_weights {
  using super = dnnl::convolution_backward_weights;

  static void compute(
      const tensor& src,
      const tensor& diff_dst,
      const dims& diff_weights_dims,
      tensor& diff_weights,
      tensor& diff_bias,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      const int groups,
      const data_type diff_weight_type = data_type::undef,
      algorithm aalgorithm = algorithm::convolution_direct,
      const engine& aengine = engine::cpu_engine()) {
    compute_impl</*with_diff_bias=*/true>(
        src,
        diff_dst,
        diff_weights_dims,
        diff_weights,
        diff_bias,
        strides,
        dilates,
        padding_l,
        padding_r,
        groups,
        diff_weight_type,
        aalgorithm,
        aengine);
  }

  static void compute(
      const tensor& src,
      const tensor& diff_dst,
      const dims& diff_weights_dims,
      tensor& diff_weights,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      const int groups,
      const data_type diff_weight_type = data_type::undef,
      algorithm aalgorithm = algorithm::convolution_direct,
      const engine& aengine = engine::cpu_engine()) {
    static tensor dummy_diff_bias;
    compute_impl</*with_diff_bias=*/false>(
        src,
        diff_dst,
        diff_weights_dims,
        diff_weights,
        dummy_diff_bias,
        strides,
        dilates,
        padding_l,
        padding_r,
        groups,
        diff_weight_type,
        aalgorithm,
        aengine);
  }

 private:
  template <bool with_diff_bias>
  static void compute_impl(
      const tensor& src,
      const tensor& diff_dst,
      const dims& diff_weights_dims,
      tensor& diff_weights,
      tensor& diff_bias,
      const dims& strides,
      const dims& dilates,
      const dims& padding_l,
      const dims& padding_r,
      const int groups,
      const data_type diff_weight_type,
      algorithm aalgorithm,
      const engine& aengine) {
    // make diff_weights and dilates compatible with DNNL
    auto dilates_ = utils::get_compatible_dilates(dilates);
    data_type diff_dst_type = diff_dst.get_data_type();
    data_type diff_weight_type_in =
        data_type::undef == diff_weight_type ? diff_dst_type : diff_weight_type;
    auto diff_weights_desc =
        tensor::desc(diff_weights_dims, diff_weight_type_in, tag::any);
    if (groups > 1) {
      diff_weights_desc = diff_weights_desc.to_grouped(groups).to_format_any();
    }

    bool channels_last = diff_dst.get_desc().is_channels_last();
    auto format_tag = channels_last
        ? (diff_dst.ndims() == 4 ? tag::nhwc : tag::ndhwc)
        : tag::any;
    auto diff_dst_desc = diff_dst.get_desc().to_format(format_tag);
    auto src_desc = src.get_desc().to_format(format_tag);

    auto diff_bias_desc =
        tensor::desc({diff_dst.get_dim(1)}, diff_weight_type_in, tag::any);

    // for forward hint, weights_desc should have same data_type
    // with other input desc, expect for bias_desc
    auto weights_desc = diff_weights_desc;
    auto forward_hints =
        convolution_forward::get_primitive_desc<with_diff_bias>(
            src_desc,
            weights_desc,
            diff_bias_desc,
            diff_dst_desc,
            strides,
            dilates_,
            padding_l,
            padding_r,
            attr_t(),
            aalgorithm,
            prop_kind::forward,
            aengine);

    auto op_attr = ideep::attr_t();
    op_attr.set_fpmath_mode();
    op_attr.set_scratchpad_mode(dnnl::scratchpad_mode::user);

    auto pd = with_diff_bias ? primitive_desc(
                                   {aalgorithm,
                                    src_desc,
                                    diff_weights_desc,
                                    diff_bias_desc,
                                    diff_dst_desc,
                                    strides,
                                    dilates_,
                                    padding_l,
                                    padding_r},
                                   op_attr,
                                   aengine,
                                   forward_hints)
                             : primitive_desc(
                                   {aalgorithm,
                                    src_desc,
                                    diff_weights_desc,
                                    diff_dst_desc,
                                    strides,
                                    dilates_,
                                    padding_l,
                                    padding_r},
                                   op_attr,
                                   aengine,
                                   forward_hints);

    auto expected_diff_dst = diff_dst.reorder_if_differ_in(pd.diff_dst_desc());
    auto expected_src = src.reorder_if_differ_in(pd.src_desc());
    // embed group info into diff_weights_desc
    auto expected_diff_weights_desc =
        tensor::desc(pd.diff_weights_desc(), groups);
    tensor expected_diff_weights;
    // diff_weights not init in FW or has same desc with expected desc.
    if (diff_weights.is_empty() ||
        diff_weights.get_desc() == expected_diff_weights_desc) {
      diff_weights.reinit_if_possible(expected_diff_weights_desc);
      expected_diff_weights = diff_weights;
    } else {
      expected_diff_weights.init(expected_diff_weights_desc);
    }

    tensor scratchpad(pd.scratchpad_desc());

    if (with_diff_bias) {
      diff_bias.reinit_if_possible(pd.diff_bias_desc());
      super(pd).execute(
          stream::default_stream(),
          {{DNNL_ARG_DIFF_DST, expected_diff_dst},
           {DNNL_ARG_SRC, expected_src},
           {DNNL_ARG_DIFF_WEIGHTS, expected_diff_weights},
           {DNNL_ARG_DIFF_BIAS, diff_bias},
           {DNNL_ARG_SCRATCHPAD, scratchpad}});
    } else {
      super(pd).execute(
          stream::default_stream(),
          {{DNNL_ARG_DIFF_DST, expected_diff_dst},
           {DNNL_ARG_SRC, expected_src},
           {DNNL_ARG_DIFF_WEIGHTS, expected_diff_weights},
           {DNNL_ARG_SCRATCHPAD, scratchpad}});
    }
    // diff_weights has been init in FW side, but has diff desc with
    // expected_diff_weights.
    if (diff_weights.get_desc() != expected_diff_weights_desc) {
      diff_weights.feed_from(expected_diff_weights);
    }
  }
};
} // namespace ideep

#endif
