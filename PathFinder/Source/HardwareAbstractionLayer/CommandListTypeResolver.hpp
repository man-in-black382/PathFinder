#pragma once

namespace HAL
{
    class DirectCommandList;
    class BundleCommandList;
    class CopyCommandList;
    class ComputeCommandList;
    class VideoProcessingCommandList;
    class VideoDecodingCommandList;

	template <class CommmandListT> class CommandListTypeResolver
	{
	public:
        static constexpr D3D12_COMMAND_LIST_TYPE ListType() { static_assert("Unsupported command list type"); }
	};
	
	template <> class CommandListTypeResolver<DirectCommandList> {
    public:
        static constexpr D3D12_COMMAND_LIST_TYPE ListType() { return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT; };
	};
	
	template <> class CommandListTypeResolver<BundleCommandList> {
    public:
        static constexpr D3D12_COMMAND_LIST_TYPE ListType() { return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_BUNDLE; };
	};
	
	template <> class CommandListTypeResolver<CopyCommandList> {
    public:
        static constexpr D3D12_COMMAND_LIST_TYPE ListType() { return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY; };
	};
	
	template <> class CommandListTypeResolver<ComputeCommandList> {
    public:
        static constexpr D3D12_COMMAND_LIST_TYPE ListType() { return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE; };
	};
	
	template <> class CommandListTypeResolver<VideoDecodingCommandList> {
    public:
        static constexpr D3D12_COMMAND_LIST_TYPE ListType() { return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE; };
	};
	
	template <> class CommandListTypeResolver<VideoProcessingCommandList> {
    public:
        static constexpr D3D12_COMMAND_LIST_TYPE ListType() { return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS; };
	};
}