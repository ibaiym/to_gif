#pragma once

#include "idecoder.hpp"
#include "iencoder.hpp"
#include "iquantizer.hpp"
#include <memory>
#include <string>

namespace to_gif {

/**
 * 流水线组件工厂接口
 * 依赖倒置原则：ParameterOptimizer 依赖此接口而非具体实现
 */
class IPipelineFactory {
public:
    virtual ~IPipelineFactory() = default;

    virtual std::unique_ptr<IDecoder> create_decoder(
        const std::string& path) = 0;

    virtual std::unique_ptr<IQuantizer> create_quantizer() = 0;

    virtual std::unique_ptr<IEncoder> create_encoder(
        const std::string& path) = 0;
};

} // namespace to_gif
