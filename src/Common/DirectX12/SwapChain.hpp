#pragma once

#include "RenderTarget.hpp"

#include <dxgi1_5.h>     // For IDXGISwapChain4
#include <wrl/client.h>  // For Microsoft::WRL::ComPtr

#include <memory>  // For std::shared_ptr

namespace DX12S
{

class SwapChain
{
public:
    static constexpr UINT NUM_BACKBUFFERS = 3;

    bool IsFullscreen() const;
    void SetFullscreen(bool fullscreen);
    bool IsVSync() const;
    void SetVSync(bool vSync);
    bool IsTearingSupported() const;

    void WaitForSwapChain();

    void Resize(UINT width, UINT height);

    /**
     * Get the render target of the window. This method should be called every
     * frame since the color attachment point changes depending on the window's
     * current back buffer.
     */
    const RenderTarget& GetRenderTarget() const;

    /**
     * Present the swapchain's back buffer to the screen.
     *
     * @param [texture] The texture to copy to the swap chain's backbuffer before
     * presenting. By default, this is an empty texture. In this case, no copy
     * will be performed. Use the SwapChain::GetRenderTarget method to get a render
     * target for the window's color buffer.
     *
     * @returns The current backbuffer index after the present.
     */
    UINT Present( const std::shared_ptr<Texture>& texture = nullptr );

    /**
     * Get the format that is used to create the backbuffer.
     */
    DXGI_FORMAT GetRenderTargetFormat() const {
        return m_RenderTargetFormat;
    }


    Microsoft::WRL::ComPtr<IDXGISwapChain4> GetDXGISwapChain() const
    {
        return m_dxgiSwapChain;
    }

protected:
    // Swap chains can only be created through the Device.
    SwapChain( Device& device, HWND hWnd, DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R10G10B10A2_UNORM );
    virtual ~SwapChain();

    // Update the swapchain's RTVs.
    void UpdateRenderTargetViews();

private:
    // The device that was used to create the SwapChain.
    Device& m_Device;

    // The command queue that is used to create the swapchain.
    // The command queue will be signaled right after the Present
    // to ensure that the swapchain's back buffers are not in-flight before
    // the next frame is allowed to be rendered.
    CommandQueue&                           m_CommandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
    std::shared_ptr<Texture>                m_BackBufferTextures[BufferCount];
    mutable RenderTarget                    m_RenderTarget;

    // The current backbuffer index of the swap chain.
    UINT   m_CurrentBackBufferIndex;
    UINT64 m_FenceValues[BufferCount];  // The fence values to wait for before leaving the Present method.

    // A handle to a waitable object. Used to wait for the swapchain before presenting.
    HANDLE m_hFrameLatencyWaitableObject;

    // The window handle that is associates with this swap chain.
    HWND m_hWnd;

    // The current width/height of the swap chain.
    uint32_t m_Width;
    uint32_t m_Height;

    // The format of the back buffer.
    DXGI_FORMAT m_RenderTargetFormat;

    // Should presentation be synchronized with the vertical refresh rate of the screen?
    // Set to true to eliminate screen tearing.
    bool m_VSync;

    // Whether or not tearing is supported.
    bool m_TearingSupported;

    // Whether the application is in full-screen exclusive mode or windowed mode.
    bool m_Fullscreen;
};

}  // namespace dx12lib
