#include "SpriteSheetEditor.h"
#include "Origin\Renderer\Renderer.h"
#include "Origin\Renderer\Renderer2D.h"
#include "Origin\Serializer\SpriteSheetSerializer.h"
#include "Origin\Asset\AssetManager.h"
#include "Origin\Scene\EntityManager.h"
#include "Origin\Core\Input.h"

#include <glm\gtc\type_ptr.hpp>
#include <yaml-cpp\yaml.h>
#include <imgui.h>

namespace origin
{
	SpriteSheetEditor::SpriteSheetEditor()
		: m_ViewportSize(0.0f)
	{
		OGN_PROFILER_UI();

		m_Camera.InitOrthographic(10.0f, 0.1f, 10.0f);
		m_Camera.SetPosition(glm::vec3(0.0f, 0.0f, 2.f));

		FramebufferSpecification spec;
		spec.Attachments =
		{
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::RED_INTEGER,
			FramebufferTextureFormat::DEPTH24STENCIL8
		};

		spec.Width = 1280;
		spec.Height = 720;

		m_Framebuffer = Framebuffer::Create(spec);
	}

	void SpriteSheetEditor::CreateNewSpriteSheet()
	{
		OGN_PROFILER_UI();

		m_SpriteSheet = SpriteSheet::Create();
	}

	void SpriteSheetEditor::SetSelectedSpriteSheet(AssetHandle handle)
	{
		OGN_PROFILER_UI();

		// Serialize before close the previous sprite
		if (!m_CurrentFilepath.empty())
		{
			Serialize(m_CurrentFilepath);
		}

		Reset();
		m_SpriteSheet = AssetManager::GetAsset<SpriteSheet>(handle);
		m_CurrentFilepath = Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilepath(handle);

		if (Deserialize() && !m_IsOpened)
		{
			m_Camera.SetOrthoSizeMax(m_Texture->GetHeight() * 1.25f);
			m_Camera.SetOrthoSize(static_cast<float>(m_Texture->GetHeight()));

			m_Camera.SetPosition(glm::vec3(0.0f, 0.0f, 2.f));
			m_IsOpened = true;
		}
	}

	void SpriteSheetEditor::SetMainTexture(AssetHandle handle)
	{
		OGN_PROFILER_UI();

		if (m_SpriteSheet)
			m_SpriteSheet->SetMainTexture(handle);
	}

	void SpriteSheetEditor::AddSprite(glm::vec2 position, glm::vec2 size, glm::vec2 min, glm::vec2 max)
	{
		OGN_PROFILER_UI();

		SpriteSheetData sprite {};
		sprite.Min = min;
		sprite.Max = max;
		m_SpriteSheet->Sprites.push_back(sprite);
	}

	void SpriteSheetEditor::RemoveSprite(int index)
	{
		OGN_PROFILER_UI();

		m_SpriteSheet->Sprites.erase(m_SpriteSheet->Sprites.begin() + index);
	}

	void SpriteSheetEditor::Duplicate(int index)
	{
		OGN_PROFILER_UI();

		m_Controls.insert(m_Controls.begin(), m_Controls[index]);
		m_SelectedIndex = 0;
	}

	void SpriteSheetEditor::OnImGuiRender()
	{
		OGN_PROFILER_UI();

		if (m_IsOpened)
		{
			ImGui::Begin("Sprite Sheet Editor", &m_IsOpened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			IsFocused = ImGui::IsWindowFocused();
			IsHovered = ImGui::IsWindowHovered();
			m_Camera.SetMoveActive(IsFocused);
			m_Camera.SetDraggingActive(IsFocused);
			m_Camera.SetScrollingActive(IsHovered);

			const ImVec2 &viewportMinRegion = ImGui::GetWindowContentRegionMin();
			const ImVec2 &viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			const ImVec2 &viewportOffset = ImGui::GetWindowPos();
			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };
			m_ViewportSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
			
			// Framebuffer Texture
			ImTextureID texture = reinterpret_cast<ImTextureID>(m_Framebuffer->GetColorAttachmentRendererID());
			ImGui::Image(texture, { m_ViewportSize.x, m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::End();

			ImGui::Begin("Sprite Sheet Inspector");

			if (m_Texture)
			{
				texture = reinterpret_cast<ImTextureID>(m_Texture->GetRendererID());
				ImVec2 atlasSize { (float)m_Texture->GetWidth(), (float)m_Texture->GetHeight() };

				if (ImGui::Button("Save"))
					Serialize(m_CurrentFilepath);

				ImGui::SameLine();
				if (ImGui::Button("Add"))
				{
					SpriteSheetController control;
					control.Size = glm::vec2(m_Camera.GetOrthoSize());
					control.Position = glm::vec2(m_Camera.GetPosition());
					m_MoveTranslation = control.Position;
					m_Controls.push_back(control);
					m_SelectedIndex = static_cast<int>(m_Controls.size()) - 1;
				}

				ImGui::Text("Atlas Size: %.2f, %.2f", atlasSize.x, atlasSize.y);

				int offset = 0;
				for (int i = 0; i < m_Controls.size(); i++)
				{
					if (m_SelectedIndex == offset / 5)
					{
						auto &c = m_Controls[m_SelectedIndex];
						c.Min = { (c.Position.x + (atlasSize.x - c.Size.x) / 2.0f) /  atlasSize.x, (c.Position.y + (atlasSize.y - c.Size.y) / 2.0f) / atlasSize.y };
						c.Max = { (c.Position.x + (atlasSize.x + c.Size.x) / 2.0f) / atlasSize.x, (c.Position.y + (atlasSize.y + c.Size.y) / 2.0f) / atlasSize.y };
					}
					offset += 5;
				}

				const float thumbnailSize = 60.0f;
				const float padding = 10.0f;
				const float cellSize = thumbnailSize + padding;
				const float panelWidth = ImGui::GetWindowContentRegionWidth();
				int columnCount = static_cast<int>(panelWidth / cellSize);
				if (columnCount < 1)
					columnCount = 1;

				ImGui::Columns(columnCount, nullptr, false);

				// SUB SPRITE IMAGES TEXTURE 
				float thumbnailHeight = thumbnailSize * ((float)atlasSize.y / (float)atlasSize.x);
				float diff = (float)(thumbnailSize - thumbnailHeight);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + diff);

				offset = 0;
				for (int i = 0; i < m_Controls.size(); i++)
				{
					auto &control = m_Controls[i];
					ImGui::PushID(i);
					ImGui::ImageButton(texture, { thumbnailSize, thumbnailSize }, { control.Min.x, control.Max.y }, { control.Max.x, control.Min.y });

					if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						m_MoveTranslation = m_Controls[i].Position;
						m_SelectedIndex = i;
					}

					if (ImGui::BeginPopupContextItem("sprite_context", 1))
					{
						if (ImGui::MenuItem("Delete"))
						{
							m_Controls.erase(m_Controls.begin() + i);
							m_SelectedIndex = !m_Controls.empty() ? i - 1 : -1;
						}

						if (ImGui::MenuItem("Duplicate"))
							Duplicate(i);

						ImGui::EndPopup();
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Position: %f, %f ", control.Position.x, control.Position.y);
						ImGui::Text("Size: %f, %f ", control.Size.x, control.Size.y);
						ImGui::Text("Min: %f, %f ", control.Min.x, control.Min.y);
						ImGui::Text("Max: %f, %f ", control.Max.x, control.Max.y);
						ImGui::EndTooltip();
					}

					if (ImGui::BeginDragDropSource())
					{
						SpriteSheetData data;
						data.Min = control.Min;
						data.Max = control.Max;
						data.TextureHandle = m_SpriteSheet->GetTextureHandle();
						ImGui::SetDragDropPayload("SPRITESHEET_ITEM", &data, sizeof(SpriteSheetData));
						ImGui::EndDragDropSource();
					}

					ImGui::NextColumn();
					ImGui::PopID();
					offset += 5;
				}
			}
			ImGui::End();
		}
	}

	void SpriteSheetEditor::OnUpdate(Timestep ts)
	{
		OGN_PROFILER_UI();

		if (!m_IsOpened)
			return;

		m_Camera.OnUpdate(ts);
		OnMouse(ts);

		RenderCommand::ClearColor(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		m_Framebuffer->Bind();
		RenderCommand::Clear();
		m_Framebuffer->ClearAttachment(1, -1);

		if (const FramebufferSpecification spec = m_Framebuffer->GetSpecification();
				m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && (m_ViewportSize.x != spec.Width || m_ViewportSize.y != spec.Height))
		{
			m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_Framebuffer->Resize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
		}

		if (m_Texture)
		{
			OGN_PROFILER_UI();

			Renderer2D::Begin(m_Camera);

			int texX = m_Texture->GetWidth();
			int texY = m_Texture->GetHeight();
			Renderer2D::DrawQuad(glm::scale(glm::mat4(1.0f), { texX, texY, 0.0f }), m_Texture);

			int offset = 0;
			for (int i = 0; i < m_Controls.size(); i++)
			{
				auto &c = m_Controls[i];

				bool selected = m_SelectedIndex == offset / 5;
				glm::mat4 tf = glm::translate(glm::mat4(1.0f), { c.Position.x, c.Position.y, 0.1f })
					* glm::scale(glm::mat4(1.0f), { c.Size.x, c.Size.y, 1.0f });
				glm::vec4 col = selected ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
				Renderer2D::DrawRect(tf, col);
				col = { 1.0f, 1.0f, 1.0f, 0.0f };
				Renderer2D::DrawQuad(tf, col, offset);

				if (selected)
				{
					float size = m_Camera.GetOrthoSize() * 0.03f;
					// bottom left corner
					glm::vec4 red = glm::vec4(0.8f, 0.1f, 0.1f, 1.0f);
					glm::vec4 green = glm::vec4(0.1f, 0.8f, 0.1f, 1.0f);
					col = c.SelectedCorner == ControllerCorner::BOTTOM_LEFT ? green : red;
					glm::mat4 tf = glm::translate(glm::mat4(1.0f), { c.Position.x - c.Size.x / 2.0f, c.Position.y - c.Size.y / 2.0f, 1.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(size));
					Renderer2D::DrawQuad(tf, col, offset + 1);

					// top left corner
					col = c.SelectedCorner == ControllerCorner::TOP_LEFT ? green : red;
					tf = glm::translate(glm::mat4(1.0f), { c.Position.x - c.Size.x / 2.0f, c.Position.y + c.Size.y / 2.0f, 1.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(size));
					Renderer2D::DrawQuad(tf, col, offset + 2);

					// bottom right corner
					col = c.SelectedCorner == ControllerCorner::BOTTOM_RIGHT ? green : red;
					tf = glm::translate(glm::mat4(1.0f), { c.Position.x + c.Size.x / 2.0f, c.Position.y - c.Size.y / 2.0f, 1.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(size));
					Renderer2D::DrawQuad(tf, col, offset + 3);

					// top right corner
					col = c.SelectedCorner == ControllerCorner::TOP_RIGHT ? green : red;
					tf = glm::translate(glm::mat4(1.0f), { c.Position.x + c.Size.x / 2.0f, c.Position.y + c.Size.y / 2.0f, 1.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(size));
					Renderer2D::DrawQuad(tf, col, offset + 4);

				}
				offset += 5;
			}

			Renderer2D::End();
		}

		auto [mx, my] = ImGui::GetMousePos();
		glm::vec2 mousePos = { mx, my };
		mousePos -= m_ViewportBounds[0];
		const glm::vec2 &viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		mousePos.y = viewportSize.y - mousePos.y;
		mousePos = glm::clamp(mousePos, glm::vec2(0.0f), viewportSize - glm::vec2(1.0f));
		m_Mouse = { static_cast<int>(mousePos.x), static_cast<int>(mousePos.y) };
		m_HoveredIndex = m_Framebuffer->ReadPixel(1, m_Mouse.x, m_Mouse.y);

		m_Framebuffer->Unbind();
	}

	bool SpriteSheetEditor::Serialize(const std::filesystem::path &filepath)
	{
		OGN_PROFILER_UI();

		m_CurrentFilepath = filepath;
		m_SpriteSheet->Sprites.clear();
		for (auto &ctrl : m_Controls)
		{
			SpriteSheetData data;
			data.Min = ctrl.Min;
			data.Max = ctrl.Max;
			m_SpriteSheet->Sprites.push_back(data);
		}

		return SpriteSheetSerializer::Serialize(filepath, m_SpriteSheet);
	}

	bool SpriteSheetEditor::Deserialize()
	{
		bool ret = SpriteSheetSerializer::Deserialize(m_CurrentFilepath, m_SpriteSheet);

		if (ret)
		{
			m_Texture = AssetManager::GetAsset<Texture2D>(m_SpriteSheet->GetTextureHandle());
			glm::vec2 atlasSize { m_Texture->GetWidth(), m_Texture->GetHeight() };
			for (auto &sprite : m_SpriteSheet->Sprites)
			{
				SpriteSheetController control;
				control.Min = sprite.Min;
				control.Max = sprite.Max;
				control.Size.x = sprite.Max.x * atlasSize.x - sprite.Min.x * atlasSize.x;
				control.Size.y = sprite.Max.y * atlasSize.y - sprite.Min.y * atlasSize.y;
				control.Position.x = sprite.Min.x * atlasSize.x - (atlasSize.x - control.Size.x) / 2.0f;
				control.Position.y = sprite.Min.y * atlasSize.y - (atlasSize.y - control.Size.y) / 2.0f;
				m_Controls.push_back(control);
			}
		}
		return ret;
	}

	void SpriteSheetEditor::OnEvent(Event &e)
	{
		OGN_PROFILER_INPUT();

		m_Camera.OnEvent(e);
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>(OGN_BIND_EVENT_FN(SpriteSheetEditor::OnMouseButtonPressed));
		dispatcher.Dispatch<KeyPressedEvent>(OGN_BIND_EVENT_FN(SpriteSheetEditor::OnKeyPressed));
	}

	bool SpriteSheetEditor::OnMouseButtonPressed(MouseButtonPressedEvent &e)
	{
		OGN_PROFILER_INPUT();

		if (e.GetMouseButton() == Mouse::ButtonLeft && IsHovered)
		{
			if (m_HoveredIndex != (m_SelectedIndex == 0 ? -1 : m_SelectedIndex) && m_HoveredIndex >= 0)
			{
				int h = m_HoveredIndex / 5;
				m_SelectedIndex = h;

				if (m_HoveredIndex == h * 5 + 1)
					m_Controls[m_SelectedIndex].SelectedCorner = BOTTOM_LEFT;
				else if (m_HoveredIndex == h * 5 + 2)
					m_Controls[m_SelectedIndex].SelectedCorner = TOP_LEFT;
				else if (m_HoveredIndex == h * 5 + 3)
					m_Controls[m_SelectedIndex].SelectedCorner = BOTTOM_RIGHT;
				else if (m_HoveredIndex == h * 5 + 4)
					m_Controls[m_SelectedIndex].SelectedCorner = TOP_RIGHT;
				else
				{
					m_Controls[m_SelectedIndex].SelectedCorner = NONE;
					m_MoveTranslation = m_Controls[m_SelectedIndex].Position;
				}
			}
			else if (m_HoveredIndex < 0 && m_SelectedIndex >= 0)
			{
				m_Controls[m_SelectedIndex].SelectedCorner = NONE;
				m_SelectedIndex = -1;
			}
		}

		return false;
	}

	bool SpriteSheetEditor::OnKeyPressed(KeyPressedEvent &e)
	{
		OGN_PROFILER_INPUT();

		if (!IsFocused)
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl);

		if (control && e.GetKeyCode() == Key::D && m_SelectedIndex >= 0 && !m_Controls.empty())
			Duplicate(m_SelectedIndex);

		return false;
	}

	void SpriteSheetEditor::OnMouse(float ts)
	{
		OGN_PROFILER_INPUT();

		if (m_Controls.empty())
			return;

		static glm::vec2 initialPosition = { 0.0f, 0.0f };
		const glm::vec2 mouse { Input::GetMouseX(), Input::GetMouseY() };
		const glm::vec2 delta = mouse - initialPosition;
		initialPosition = mouse;

		if (Input::IsMouseButtonPressed(Mouse::ButtonLeft) && IsHovered && m_SelectedIndex >= 0)
		{
			auto &c = m_Controls[m_SelectedIndex];
			float orthoScale = m_Camera.GetOrthoSize() / m_Camera.GetHeight();

			glm::vec2 atlasSize = { (float)m_Texture->GetWidth(), (float)m_Texture->GetHeight() };

			m_MoveTranslation.x += delta.x * orthoScale;
			m_MoveTranslation.y -= delta.y * orthoScale;

			float snapSize = 0.5f;
			if (Input::IsKeyPressed(Key::LeftShift))
			{
				if (Input::IsKeyPressed(Key::LeftControl))
					snapSize = 0.1f;

				switch (c.SelectedCorner)
				{
					case NONE:
						c.Position.x = round(m_MoveTranslation.x / snapSize) * snapSize;
						c.Position.y = round(m_MoveTranslation.y / snapSize) * snapSize;
						break;
					default:
						break;
				}
			}
			else
			{
				switch (c.SelectedCorner)
				{
					case NONE:
						if (Input::IsKeyPressed(Key::X))
							c.Position.x += delta.x * orthoScale;
						else if (Input::IsKeyPressed(Key::Y))
							c.Position.y -= delta.y * orthoScale;
						else
						{
							c.Position.x += delta.x * orthoScale;
							c.Position.y -= delta.y * orthoScale;
						}
						break;
					case TOP_RIGHT:
						c.Position.x += delta.x * orthoScale / 2.0f;
						c.Size.x += delta.x * orthoScale;
						c.Position.y -= delta.y * orthoScale / 2.0f;
						c.Size.y -= delta.y * orthoScale;
						break;
					case BOTTOM_RIGHT:
						c.Position.x += delta.x * orthoScale / 2.0f;
						c.Size.x += delta.x * orthoScale;
						c.Position.y -= delta.y * orthoScale / 2.0f;
						c.Size.y += delta.y * orthoScale;
						break;
					case TOP_LEFT:
						c.Position.x += delta.x * orthoScale / 2.0f;
						c.Size.x -= delta.x * orthoScale;
						c.Position.y -= delta.y * orthoScale / 2.0f;
						c.Size.y -= delta.y * orthoScale;
						break;
					case BOTTOM_LEFT:
						c.Position.x += delta.x * orthoScale / 2.0f;
						c.Size.x -= delta.x * orthoScale;
						c.Position.y -= delta.y * orthoScale / 2.0f;
						c.Size.y += delta.y * orthoScale;
						break;
				}
			}
		}
	}

	void SpriteSheetEditor::Reset()
	{
		OGN_PROFILER_UI();

		if (m_SpriteSheet)
		{
			m_SpriteSheet->Sprites.clear();
			m_SpriteSheet.reset();
		}

		m_Controls.clear();
	}
}