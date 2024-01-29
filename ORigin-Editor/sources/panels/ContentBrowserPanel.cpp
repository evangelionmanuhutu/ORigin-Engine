// Copyright (c) Evangelion Manuhutu | ORigin Engine
#include "ContentBrowserPanel.h"
#include "../EditorLayer.h"

#include "Origin/Asset/AssetImporter.h"
#include "Origin/Project/Project.h"
#include "Origin/Utils/Utils.h"
#include "Origin\Utils\StringUtils.h"

#include <imgui.h>

#pragma warning(disable : OGN_DISABLED_WARNINGS)

namespace origin
{
	static uint32_t itemRenderCount = 0;

	ContentBrowserPanel::ContentBrowserPanel(const std::shared_ptr<Project>& project)
		: m_Project(project), m_ThumbnailCache(std::make_shared<ThumbnailCache>(project)), m_BaseDirectory(m_Project->GetAssetDirectory()), m_CurrentDirectory(m_BaseDirectory)
	{
		m_TreeNodes.push_back(TreeNode(".", 0));
		m_IconMap["backward_button_icon"] = TextureImporter::LoadTexture2D("Resources/UITextures/backward_icon.png");
		m_IconMap["forward_button_icon"] = TextureImporter::LoadTexture2D("Resources/UITextures/forward_icon.png");
		m_IconMap["directory_icon"] = TextureImporter::LoadTexture2D("Resources/UITextures/folder_icon.png");
		m_IconMap["asset_mode_icon"] = TextureImporter::LoadTexture2D("Resources/UITextures/asset_mode_icon.png");
		m_IconMap["file_mode_icon"] = TextureImporter::LoadTexture2D("Resources/UITextures/file_mode_icon.png");
		m_IconMap["unknown"] = TextureImporter::LoadTexture2D("Resources/UITextures/file_icon.png");
		m_IconMap[".jpg"] = TextureImporter::LoadTexture2D("Resources/UITextures/jpg_file_icon.png");
		m_IconMap[".cs"] = TextureImporter::LoadTexture2D("Resources/UITextures/script_icon.png");
		m_IconMap[".glsl"] = TextureImporter::LoadTexture2D("Resources/UITextures/glsl_icon.png");
		m_IconMap[".hlsl"] = TextureImporter::LoadTexture2D("Resources/UITextures/hlsl_file_icon.png");
		m_IconMap[".org"] = TextureImporter::LoadTexture2D("Resources/UITextures/scene_icon.png");
		m_IconMap[".mp3"] = TextureImporter::LoadTexture2D("Resources/UITextures/audio_icon.png");
		
		RefreshAssetTree();
		m_Mode = Mode::FileSystem;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		if (!m_Project)
			return;

		DrawContentBrowser();
	}

	void ContentBrowserPanel::DrawNavButton()
	{
		ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
		ImGui::BeginChild("navigation_button", ImVec2(ImGui::GetContentRegionAvail().x, 30.0f), false, childFlags);

		// Navigation Button
		ImVec2 navBtSize = ImVec2(23.0f, 23.0f);
		
		// Push Button Colors
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		// Backward Button
		std::shared_ptr<Texture2D> navButtonTexture = m_IconMap.at("backward_button_icon");
		if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(navButtonTexture->GetRendererID()), navBtSize, ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (m_CurrentDirectory != Project::GetActiveAssetDirectory())
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
				m_ForwardCount--;
			}
		}
		
		// Forward button
		ImGui::SameLine();
		navButtonTexture = m_IconMap.at("forward_button_icon");
		ImGui::ImageButton(reinterpret_cast<ImTextureID>(navButtonTexture->GetRendererID()), navBtSize, ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if(!m_HistoryList.empty())
			{
				//m_CurrentDirectory /= m_HistoryList.at(m_ForwardCount);
				//m_ForwardCount++;
			}
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		// Push Button Colors
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_Mode == Mode::Asset ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(0.8f, 0.0f, 0.8f, 1.0f));
		const char* label = m_Mode == Mode::Asset ? "Asset" : "File";
		navButtonTexture = m_Mode == Mode::Asset ? m_IconMap.at("asset_mode_icon") : m_IconMap.at("file_mode_icon");
		if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(navButtonTexture->GetRendererID()), navBtSize, ImVec2(0, 1), ImVec2(1, 0)))
		{
			m_Mode = m_Mode == Mode::Asset ? Mode::FileSystem : Mode::Asset;
			RefreshAssetTree();
		}
		ImGui::PopStyleColor(3);
		ImGui::EndChild();
	}

	void ContentBrowserPanel::DrawContentBrowser()
	{
		ImGui::Begin("Content Browser");
		DrawNavButton();

		ImGui::BeginChild("item_list", ImVec2(ImGui::GetContentRegionAvail()), false);

		static float padding = 10.0f;
		const float cellSize = m_ThumbnailSize + padding;
		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = static_cast<int>(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		if (m_Mode == Mode::Asset)
		{
			TreeNode* node = m_TreeNodes.data();
			const auto& currentDir = std::filesystem::relative(m_CurrentDirectory, Project::GetActiveAssetDirectory());
			for (const auto& path : currentDir)
			{
				if (node->Path == currentDir)
					break;

				if (node->Children.find(path) != node->Children.end())
					node = &m_TreeNodes[node->Children[path]];
			}

			for (const auto& [item, treeNodeIndex] : node->Children)
			{
				bool shouldBreak = false;
				const bool isDirectory = std::filesystem::is_directory(Project::GetActiveAssetDirectory() / item);
				std::string filenameStr = item.generic_string();

				ImGui::PushID(filenameStr.c_str());

				const std::shared_ptr<Texture2D> thumbnail = DirectoryIcons(std::filesystem::directory_entry(m_CurrentDirectory / item));
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

				float thumbnailHeight = m_ThumbnailSize * ((float)thumbnail->GetHeight() / (float)thumbnail->GetWidth());
				float diff = (float)(m_ThumbnailSize - thumbnailHeight);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + diff);

				ImGui::ImageButton(reinterpret_cast<ImTextureID>(thumbnail->GetRendererID()), { (float)m_ThumbnailSize, (float)thumbnailHeight }, { 0, 1 }, { 1, 0 });

				if (ImGui::BeginDragDropSource())
				{
					AssetHandle handle = m_TreeNodes[treeNodeIndex].Handle;
					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", &handle, sizeof(AssetHandle));
					ImGui::EndDragDropSource();
				}

				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (isDirectory)
					{
						m_CurrentDirectory /= item.filename();
						m_HistoryList.insert({ m_HistoryList.size(), m_CurrentDirectory });
						m_ForwardCount++;
					}
				}

				if (ImGui::BeginPopupContextItem())
				{
					Utils::CenteredText(Utils::CapitalizeWholeText(filenameStr).c_str());
					ImGui::Separator();

					if (item.extension() == ".org")
					{
						if (ImGui::MenuItem("Set As Start Scene"))
						{
							AssetHandle handle = m_TreeNodes[treeNodeIndex].Handle;
							Project::GetActive()->SetStartScene(handle);
							RefreshAssetTree();
						}
					}

					if (ImGui::MenuItem("Remove From Project"))
					{
						AssetHandle handle = m_TreeNodes[treeNodeIndex].Handle;
						Project::GetActive()->GetEditorAssetManager()->RemoveAsset(handle);

						uint32_t index = treeNodeIndex;

						node->Children.erase(item);
						m_TreeNodes.erase(m_TreeNodes.begin() + index);

						// reset the node
						uint32_t count = 0;
						for (auto n : m_TreeNodes)
						{
							if (node->Children.find(n.Path) != node->Children.end())
								node->Children[n.Path] = count;
							count++;
						}

						shouldBreak = true;
						RefreshAssetTree();
					}
					ImGui::EndPopup();
				}

				ImGui::TextWrapped(filenameStr.c_str());
				ImGui::NextColumn();

				ImGui::PopID();

				if (shouldBreak)
					break;
			}
		}

		// ==== File Mode ====
		else if(m_Mode == Mode::FileSystem)
		{
			uint32_t count = 0;
			for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				count++;
			}

			ImGuiListClipper clipper(glm::ceil((float)count / (float)columnCount));
			bool first = true;
			while (clipper.Step())
			{
				auto it = std::filesystem::directory_iterator(m_CurrentDirectory);
				if (!first)
				{
					for (int i = 0; i < clipper.DisplayStart; i++)
					{
						for (int c = 0; c < columnCount && it != std::filesystem::directory_iterator(); c++)
						{
							it++;
						}
					}
				}

				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					int c;
					for (c = 0; c < columnCount && it != std::filesystem::directory_iterator(); c++, it++)
					{
						const auto& directoryEntry = *it;

						const auto& path = directoryEntry.path();
						const std::string filenameStr = path.filename().string();

						ImGui::PushID(filenameStr.c_str());

						const auto& relativePath = std::filesystem::relative(path, Project::GetActiveAssetDirectory());
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
						auto& thumbnail = DirectoryIcons(directoryEntry);
						float thumbnailHeight = m_ThumbnailSize * ((float)thumbnail->GetHeight() / (float)thumbnail->GetWidth());
						float diff = (float)(m_ThumbnailSize - thumbnailHeight);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + diff);

						ImGui::ImageButton(reinterpret_cast<ImTextureID>(thumbnail->GetRendererID()), { (float)m_ThumbnailSize, (float)thumbnailHeight }, { 0, 1 }, { 1, 0 });
						ImGui::PopStyleColor();

						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							std::string sizeString = Utils::BytesToString(thumbnail->GetEstimatedSize());
							ImGui::Text("Mem : %s", sizeString.c_str());
							ImGui::EndTooltip();
						}

						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							if (directoryEntry.is_directory())
							{
								m_CurrentDirectory /= path.filename();
								m_HistoryList.insert({ m_HistoryList.size(), m_CurrentDirectory });
							}
						}

						if (ImGui::BeginPopupContextItem())
						{
							Utils::CenteredText(Utils::CapitalizeWholeText(filenameStr).c_str());
							ImGui::Separator();

							bool assetImported = false;
							const auto& assetRegistry = Project::GetActive()->GetEditorAssetManager()->GetAssetRegistry();
							for (const auto& [handle, metadata] : assetRegistry)
							{
								if (relativePath.generic_string() == metadata.Filepath)
								{
									assetImported = true;
									break;
								}
								assetImported = false;
							}

							if (!assetImported)
							{
								if (ImGui::MenuItem("Import To Project"))
								{
									Project::GetActive()->GetEditorAssetManager()->ImportAsset(relativePath);
									RefreshAssetTree();
								}
							}

							ImGui::EndPopup();
						}

						ImGui::TextWrapped(filenameStr.c_str());
						ImGui::NextColumn();

						ImGui::PopID();
					}

					if (first && c != columnCount)
					{
						for (int i = 0; i < columnCount - c; i++)
						{
							ImGui::NextColumn();
						}
					}
					first = false;
				}
			}
		}

		ImGui::Columns(1);

		// Right Click Context
		if (ImGui::BeginPopupContextWindow(nullptr, 1, false))
		{
			if (ImGui::BeginMenu("CREATE"))
			{
				if (ImGui::MenuItem("New Folder", nullptr))
				{
					std::string folderName = "New Folder";
					const auto newFolderPath = m_CurrentDirectory / folderName;
					if (!std::filesystem::exists(newFolderPath))
						std::filesystem::create_directory(newFolderPath);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Thumbnail Size"))
			{
				if (ImGui::MenuItem("Small"))
					m_ThumbnailSize = 70;
				if (ImGui::MenuItem("Medium"))
					m_ThumbnailSize = 90;
				if (ImGui::MenuItem("Large"))
					m_ThumbnailSize = 110;

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		ImGui::EndChild();

		ImGui::End();
		m_ThumbnailCache->OnUpdate();
	}

	void ContentBrowserPanel::RefreshAssetTree()
	{
		const auto& assetRegistry = Project::GetActive()->GetEditorAssetManager()->GetAssetRegistry();
		for (const auto& [handle, metadata] : assetRegistry)
		{
			uint32_t currentNodeIndex = 0;
			for (const auto& p : metadata.Filepath)
			{
				auto it = m_TreeNodes[currentNodeIndex].Children.find(p.generic_string());
				if (it != m_TreeNodes[currentNodeIndex].Children.end())
				{
					currentNodeIndex = it->second;
				}
				else
				{
					TreeNode newNode(p, handle);
					newNode.Parent = currentNodeIndex;

					m_TreeNodes.push_back(newNode);
					m_TreeNodes[currentNodeIndex].Children[p] = m_TreeNodes.size() - 1;

					currentNodeIndex = m_TreeNodes.size() - 1;
				}
			}
		}
	}

	std::shared_ptr<Texture2D> ContentBrowserPanel::DirectoryIcons(const std::filesystem::directory_entry& dirEntry)
	{
		const std::string& fileExtension = dirEntry.path().extension().string();
		auto relativePath = std::filesystem::relative(dirEntry.path(), Project::GetActiveAssetDirectory());

		std::shared_ptr<Texture2D> texture = m_IconMap.at("directory_icon");

		if (!dirEntry.is_directory())
		{
			if (fileExtension == ".png" || fileExtension == ".jpg")
			{
				texture = m_ThumbnailCache->GetOrCreateThumbnail(relativePath);
				if (!texture)
					texture = m_IconMap.at("unknown");
			}
			else if (m_IconMap.find(fileExtension) == m_IconMap.end())
				texture = m_IconMap.at("unknown");
			else
				texture = m_IconMap.at(fileExtension);
		}

		return texture;
	}

}
