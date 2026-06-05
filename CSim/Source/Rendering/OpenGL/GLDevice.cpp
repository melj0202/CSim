#include "GLDevice.h"

void GLDevice::ApplyPipelineState(const PipelineState& pipelineState)
{

// 1. Depth Testing optimization
	if (pipelineState.depthTestEnabled != _currentGLState.depthTestEnabled)
	{
		pipelineState.depthTestEnabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
		_currentGLState.depthTestEnabled = pipelineState.depthTestEnabled;
	}

	// 2. Blending logic and state reset
	if (pipelineState.blendEnabled != _currentGLState.blendEnabled)
	{
		pipelineState.blendEnabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
		_currentGLState.blendEnabled = pipelineState.blendEnabled;
	}
	// Even if blending status didn't toggle, check if factors changed while active
	if (pipelineState.blendEnabled)
	{
		if (pipelineState.blendSrc != _currentGLState.blendSrc || pipelineState.blendDst != _currentGLState.blendDst)
		{
			glBlendFunc(mapBlendFactor(pipelineState.blendSrc), mapBlendFactor(pipelineState.blendDst));
			_currentGLState.blendSrc = pipelineState.blendSrc;
			_currentGLState.blendDst = pipelineState.blendDst;
		}
	}

	// 3. Face Culling safe state sync
	if (pipelineState.faceCullingEnabled != _currentGLState.faceCullingEnabled)
	{
		pipelineState.faceCullingEnabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
		_currentGLState.faceCullingEnabled = pipelineState.faceCullingEnabled;
	}
	if (pipelineState.faceCullingEnabled)
	{
		if (pipelineState.cullFace != _currentGLState.cullFace)
		{
			glCullFace(mapCullMode(pipelineState.cullFace));
			_currentGLState.cullFace = pipelineState.cullFace;
		}
		if (pipelineState.frontFace != _currentGLState.frontFace)
		{
			glFrontFace(mapWindingOrder(pipelineState.frontFace));
			_currentGLState.frontFace = pipelineState.frontFace;
		}
	}

	// 4. Wireframe state change optimization
	if (pipelineState.wireframe != _currentGLState.wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, pipelineState.wireframe ? GL_LINE : GL_FILL);
		_currentGLState.wireframe = pipelineState.wireframe;
	}
}

void GLDevice::ExecuteCommandQueue(CommandQueue& commandQueue)
{
	for (size_t i = 0; i < commandQueue.GetCommandCount(); ++i)
	{
		RenderCommand& cmd = commandQueue.GetCommand(i);
		ApplyPipelineState(cmd.pipelineState);
		switch (cmd.commandType)
		{
			case CommandType::Draw:
				glDrawArrays(GL_TRIANGLES, 0, cmd.elementCount);
				break;
			case CommandType::DrawIndexed:
				glDrawElements(GL_TRIANGLES, cmd.elementCount, GL_UNSIGNED_INT, nullptr);
				break;
			case CommandType::DrawInstanced:
				glDrawArraysInstanced(GL_TRIANGLES, 0, cmd.elementCount, 1);
				break;
			case CommandType::DrawBatched:
				break;
			case CommandType::SetShader:
				glUseProgram(cmd.resourceHandle);
				break;
			case CommandType::SetVertexBuffer:
				glBindBuffer(GL_ARRAY_BUFFER, cmd.resourceHandle);
				break;
			case CommandType::SetIndexBuffer:
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.resourceHandle);
				break;
			case CommandType::SetTexture:
				glActiveTexture(GL_TEXTURE0 + cmd.slot);
				glBindTexture(GL_TEXTURE_2D, cmd.resourceHandle);
				break;
			case CommandType::SetViewport:
			   // glViewport(cmd.x, cmd.y, cmd.width, cmd.height);
				break;
			case CommandType::ClearScreen:
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				break;
			case CommandType::ClearDepthBuffer:
				glClear(GL_DEPTH_BUFFER_BIT);
				break;
			case CommandType::ClearColorBuffer:
				glClear(GL_COLOR_BUFFER_BIT);
				break;
			case CommandType::ClearStencilBuffer:
				glClear(GL_STENCIL_BUFFER_BIT);
				break;
			case CommandType::ClearAll:
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				break;
			default:
				break;
		}
	}
}