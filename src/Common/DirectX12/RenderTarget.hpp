#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "Texture.hpp"

namespace DX12S
{

enum class AttachmentPoint : int
{
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    DepthStencil,
    NumAttachmentPoints
};

class RenderTarget
{
public:
    RenderTarget();

    RenderTarget(const RenderTarget &copy) = default;
    RenderTarget(RenderTarget &&copy) = default;

    RenderTarget & operator=(const RenderTarget &other) = default;
    RenderTarget & operator=(RenderTarget &&other) = default;

    void AttachTexture(AttachmentPoint attachmentPoint, std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> GetTexture(AttachmentPoint attachmentPoint) const;

    void Resize(uint32_t width, uint32_t height);
    void GetWidth(uint32_t &width, uint32_t &height);

    // Get a list of the textures attached to the render target.
    // This method is primarily used by the CommandList when binding the
    // render target to the output merger stage of the rendering pipeline.
    const std::vector<std::shared_ptr<Texture>>& GetTextures() const;

    // Get the render target formats of the textures currently
    // attached to this render target object.
    // This is needed to configure the Pipeline state object.
    D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

    // Get the format of the attached depth/stencil buffer.
    DXGI_FORMAT GetDepthStencilFormat() const;

    // Get the sample description of the render target.
    DXGI_SAMPLE_DESC GetSampleDesc() const;

    // Reset all textures
    void Reset()
    {
        m_Textures = RenderTargetList( AttachmentPoint::NumAttachmentPoints );
    }

private:
    using RenderTargetList = std::vector<std::shared_ptr<Texture>>;
    RenderTargetList m_Textures;
    DirectX::XMUINT2                      m_Size;
};
}  // namespace dx12lib