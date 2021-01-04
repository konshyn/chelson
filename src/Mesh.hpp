#pragma once

#include <DirectXCollision.h>  // For BoundingBox
#include <DirectXMath.h>       // For XMFLOAT3, XMFLOAT2

#include <d3d12.h>  // For D3D12_INPUT_LAYOUT_DESC, D3D12_INPUT_ELEMENT_DESC

#include <map>     // For std::map
#include <memory>  // For std::shared_ptr

namespace dx12lib
{

class CommandList;
class IndexBuffer;
class Material;
class VertexBuffer;
class Visitor;

class Mesh
{
public:
    using BufferMap = std::map<uint32_t, std::shared_ptr<VertexBuffer>>;

    Mesh();
    ~Mesh() = default;

    void                     SetPrimitiveTopology( D3D12_PRIMITIVE_TOPOLOGY primitiveToplogy );
    D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const;

    void                          SetVertexBuffer( uint32_t slotID, const std::shared_ptr<VertexBuffer>& vertexBuffer );
    std::shared_ptr<VertexBuffer> GetVertexBuffer( uint32_t slotID ) const;
    const BufferMap&              GetVertexBuffers() const
    {
        return m_VertexBuffers;
    }

    void                         SetIndexBuffer( const std::shared_ptr<IndexBuffer>& indexBuffer );
    std::shared_ptr<IndexBuffer> GetIndexBuffer();

    /**
     * Get the number if indicies in the index buffer.
     * If no index buffer is bound to the mesh, this function returns 0.
     */
    size_t GetIndexCount() const;

    /**
     * Get the number of verticies in the mesh.
     * If this mesh does not have a vertex buffer, the function returns 0.
     */
    size_t GetVertexCount() const;

    void                      SetMaterial( std::shared_ptr<Material> material );
    std::shared_ptr<Material> GetMaterial() const;

    /**
     * Set the AABB bounding volume for the geometry in this mesh.
     */
    void                        SetAABB( const DirectX::BoundingBox& aabb );
    const DirectX::BoundingBox& GetAABB() const;

    /**
     * Draw the mesh to a CommandList.
     *
     * @param commandList The command list to draw to.
     * @param instanceCount The number of instances to draw.
     * @param startInstance The offset added to the instance ID when reading from the instance buffers.
     */
    void Draw( CommandList& commandList, uint32_t instanceCount = 1, uint32_t startInstance = 0 );

    /**
     * Accept a visitor.
     */
    void Accept( Visitor& visitor );

private:
    BufferMap                    m_VertexBuffers;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
    std::shared_ptr<Material>    m_Material;
    D3D12_PRIMITIVE_TOPOLOGY     m_PrimitiveTopology;
    DirectX::BoundingBox         m_AABB;
};
}  // namespace dx12lib