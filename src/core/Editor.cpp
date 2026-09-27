#include "core/Editor.h"


#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include "core/Log.h"
#include "core/viewport.h"
#include "core/Window.h"

#include "renderer/IBL.h"
#include "renderer/Camera.h"
#include "utils/RenderProfiler.h"

#include "scene/GameObject.h"
#include "scene/Scene.h"

#include "components/SkyAtmosphere.h"
#include "components/VolumetricCloud.h"

#include "utils/Transform.h"

#include "renderer/IBL.h"
#include <glm/gtc/matrix_transform.hpp>



void Editor::ShowMaterialPreview()
{
	const auto& preview = materialPreview;

	if (!preview.texture) return;

	// 必须紧接在场景的 ImGui::Image() 后调用。
	const ImVec2 viewMin = ImGui::GetItemRectMin();
	const ImVec2 viewMax = ImGui::GetItemRectMax();

	const float padding = 12.0f;
	const float availableWidth = viewMax.x - viewMin.x;
	const float availableHeight = viewMax.y - viewMin.y;

	if (availableWidth <= 0.0f || availableHeight <= 0.0f) return;

	const float scale = std::min(1.0f, std::min(availableWidth / float(preview.width), availableHeight / float(preview.height)));
	const ImVec2 size(float(preview.width) * scale, float(preview.height) * scale);

	const ImVec2 bottomRight(viewMax.x, viewMax.y);
	const ImVec2 topLeft(bottomRight.x - size.x, bottomRight.y - size.y);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->PushClipRect(viewMin, viewMax, true);
	drawList->AddImage((ImTextureID)(intptr_t)preview.texture, topLeft, bottomRight, ImVec2(0, 1), ImVec2(1, 0));
	drawList->PopClipRect();
}




void Editor::InitMaterialPreview()
{
	auto& preview = materialPreview;

	if (preview.FBO || preview.width <= 0 || preview.height <= 0) return;

	GLint previousDrawFBO = 0;
	GLint previousReadFBO = 0;
	GLint previousTexture = 0;
	GLint previousRenderbuffer = 0;

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFBO);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFBO);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);

	glGenFramebuffers(1, &preview.FBO);
	glGenTextures(1, &preview.texture);
	glGenRenderbuffers(1, &preview.RBO);

	glBindTexture(GL_TEXTURE_2D, preview.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, preview.width, preview.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindRenderbuffer(GL_RENDERBUFFER, preview.RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, preview.width, preview.height);

	glBindFramebuffer(GL_FRAMEBUFFER, preview.FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, preview.texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, preview.RBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFBO);
	glBindTexture(GL_TEXTURE_2D, previousTexture);
	glBindRenderbuffer(GL_RENDERBUFFER, previousRenderbuffer);

	if (!complete)
	{
		LOG_ERROR("Material preview framebuffer incomplete");

		glDeleteFramebuffers(1, &preview.FBO);
		glDeleteTextures(1, &preview.texture);
		glDeleteRenderbuffers(1, &preview.RBO);

		preview.FBO = preview.texture = preview.RBO = 0;
	}
}

void Editor::RenderMaterialPreview(const IBL& ibl, Camera& camera)
{
	auto& preview = materialPreview;

	if (!ibl.irradianceMap || !ibl.prefilterMap || !ibl.brdfLUTTexture) return;

	if (!preview.FBO) InitMaterialPreview();
	if (!preview.FBO || preview.width <= 0 || preview.height <= 0) return;

	GLint previousDrawFBO = 0;
	GLint previousReadFBO = 0;
	GLint previousViewport[4];
	GLint previousDepthFunc = 0;
	GLint previousProgram = 0;
	GLint previousVAO = 0;
	GLint previousActiveTexture = 0;
	GLint previousRenderbuffer = 0;
	GLint previousCubeTextures[3];
	GLint previous2DTextures[3];

	GLfloat previousClearColor[4];
	GLdouble previousClearDepth = 1.0;
	GLboolean previousDepthMask = GL_TRUE;
	GLboolean previousColorMask[4];

	const GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean previousBlend = glIsEnabled(GL_BLEND);
	const GLboolean previousCull = glIsEnabled(GL_CULL_FACE);
	const GLboolean previousScissor = glIsEnabled(GL_SCISSOR_TEST);
	const GLboolean previousSRGB = glIsEnabled(GL_FRAMEBUFFER_SRGB);

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFBO);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFBO);
	glGetIntegerv(GL_VIEWPORT, previousViewport);
	glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
	glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);

	glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);
	glGetDoublev(GL_DEPTH_CLEAR_VALUE, &previousClearDepth);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
	glGetBooleanv(GL_COLOR_WRITEMASK, previousColorMask);

	for (int i = 0; i < 3; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousCubeTextures[i]);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous2DTextures[i]);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, preview.FBO);
	glViewport(0, 0, preview.width, preview.height);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_FRAMEBUFFER_SRGB);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const glm::vec3 cameraPosition(0.0f, 0.0f, 10.0f);
	const glm::mat4 projection = glm::perspective(glm::radians(15.0f), float(preview.width) / float(preview.height), 0.1f, 20.0f);

	ibl.Bind(preview.shader);

	const glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	preview.shader.setMat4("view", view);
	preview.shader.setMat4("projection", projection);
	preview.shader.setVec3("viewPos", cameraPosition);
	
	const glm::vec3 direction = glm::normalize(camera.GetDirection());
	const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), direction));
	const glm::vec3 up = glm::normalize(glm::cross(direction, right));

	preview.shader.setMat3("environmentRotation", glm::mat3(right, up, direction));

	preview.shader.setVec3("baseColor", glm::vec3(0.5f));

	preview.shader.setVec3("lightPos", glm::vec3(0.0f, 3.0f, 4.0f));
	preview.shader.setVec3("light.intensity", glm::vec3(0.0f));
	
	const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));

	preview.shader.setMat4("model", model);
	preview.shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
	preview.shader.setFloat("metallic", preview.metallic);
	preview.shader.setFloat("roughness", preview.roughness);

	preview.sphereModel.Draw(preview.shader);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFBO);
	glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

	glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
	glClearDepth(previousClearDepth);
	glDepthFunc(previousDepthFunc);
	glDepthMask(previousDepthMask);
	glColorMask(previousColorMask[0], previousColorMask[1], previousColorMask[2], previousColorMask[3]);

	if (!previousDepthTest) glDisable(GL_DEPTH_TEST);
	if (previousBlend) glEnable(GL_BLEND);
	if (previousCull) glEnable(GL_CULL_FACE);
	if (previousScissor) glEnable(GL_SCISSOR_TEST);
	if (previousSRGB) glEnable(GL_FRAMEBUFFER_SRGB);

	for (int i = 0; i < 3; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_CUBE_MAP, previousCubeTextures[i]);
		glBindTexture(GL_TEXTURE_2D, previous2DTextures[i]);
	}

	glActiveTexture(previousActiveTexture);
	glUseProgram(previousProgram);
	glBindVertexArray(previousVAO);
	glBindRenderbuffer(GL_RENDERBUFFER, previousRenderbuffer);
}

void Editor::Init(Window* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	currentScale = window->GetCurrentScale();

	this->font_name = "assets/fonts/Inter-Medium.ttf";
	this->font_small = io.Fonts->AddFontFromFileTTF(this->font_name.c_str(), 13.f * currentScale);
    // io.Fonts->AddFontFromFileTTF("assets/fonts/Inter-Medium.ttf", 14.0f * xscale);

    ImGui::GetStyle().ScaleAllSizes(currentScale);
    LOG_INFO("editor created x{} scale", currentScale);
    ImGui_ImplGlfw_InitForOpenGL(window->GetGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init();
}

void Editor::Update(const Window& window)
{
	const float newScale = window.GetCurrentScale();
	if (newScale <= 0.0f || std::abs(newScale - currentScale) < 0.001f) {
		return;
	}

	const float scaleRatio = newScale / currentScale;
	ImGui::GetStyle().ScaleAllSizes(scaleRatio);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	this->font_small = io.Fonts->AddFontFromFileTTF(this->font_name.c_str(), 13.f * newScale);
	io.FontDefault = font_small;

	currentScale = newScale;
	LOG_INFO("Editor UI scale changed to {}", currentScale);
}

Editor::~Editor()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}



void Editor::BeginFrame(Viewport& viewport)
{
    ImGui_ImplOpenGL3_NewFrame();
    
	ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
	ImGui::DockSpaceOverViewport();


	// 1.1 视窗
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // style

	ImGui::Begin("Viewport");

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	viewport.Resize(viewportSize.x, viewportSize.y);

	ImGui::Image((void*)(intptr_t)viewport.GetColorTexture(), ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
	this->isViewportHovered = ImGui::IsItemHovered();

	ShowMaterialPreview();

	ImGui::End();

	ImGui::PopStyleVar(); // style
}

void Editor::BeginCamera(Camera & camera) {
	ImGui::Begin("Camera");

	ImGui::Text("View:");

	auto fovToMm = [](float fov_deg, float sensor_diag = 43.27f) -> float {
		float fov_rad = glm::radians(fov_deg);
		return (sensor_diag / 2.0f) / std::tan(fov_rad / 2.0f);
	};

	auto mmToFov = [](float mm, float sensor_diag = 43.27f) -> float {
		return glm::degrees(2.0f * std::atan((sensor_diag / 2.0f) / mm));
	};

	float fov = camera.GetFov();
	float mm = fovToMm(fov);

	if (ImGui::SliderFloat("Focal Length", &mm, fovToMm(170.0f), fovToMm(10.0f), "%.1f mm")) {
		mm = glm::clamp(mm, 1.0f, 500.0f); // 防止极端值
		camera.SetFov(mmToFov(mm));
	}

	ImGui::Text("Movement:");

	glm::vec3 position = camera.GetPosition();
	if (ImGui::InputFloat3("Position", &position.x, "%.2f")) {
		camera.SetPosition(position);
	}

	glm::vec3 direction = camera.GetDirection();
	if (ImGui::InputFloat3("Direction", &direction.x, "%.3f")) {
		camera.SetDirection(direction);
	}
	ImGui::InputFloat("Speed", &camera.moveSpeed, 0.5f, 1.0f, "%.1f");


	ImGui::End();
}

void Editor::BeginIBL() {
	if (!ImGui::Begin("IBL")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Test Sphere Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Metallic", &materialPreview.metallic, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Roughness", &materialPreview.roughness, 0.01f, 1.0f, "%.2f");
	}

	ImGui::End();
}

void Editor::BeginDetails(GameObject& game_object) {
	Transform& transform = game_object.GetTransform();

	if (!ImGui::Begin("Details")) {
		ImGui::End();
		return;
	}

	ImGui::Text(game_object.GetName().c_str());

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Location:");
		glm::vec3 position = transform.position;
		bool positionChanged = false;
		positionChanged |= ImGui::DragFloat("X##trans_x", &position[0], 0.005f, -FLT_MAX, +FLT_MAX, "%.2f m");
		positionChanged |= ImGui::DragFloat("Y##trans_y", &position[1], 0.005f, -FLT_MAX, +FLT_MAX, "%.2f m");
		positionChanged |= ImGui::DragFloat("Z##trans_z", &position[2], 0.005f, -FLT_MAX, +FLT_MAX, "%.2f m");
		if (positionChanged)
			transform.SetPosition(position);
		ImGui::Spacing();

		ImGui::Text("Rotation:");
		glm::vec3 rotation = transform.rotation;
		bool rotationChanged = false;
		rotationChanged |= ImGui::DragFloat("X##rotate_x", &rotation[0], 0.2f, -FLT_MAX, +FLT_MAX, "%.0f deg");
		rotationChanged |= ImGui::DragFloat("Y##rotate_y", &rotation[1], 0.2f, -FLT_MAX, +FLT_MAX, "%.0f deg");
		rotationChanged |= ImGui::DragFloat("Z##rotate_z", &rotation[2], 0.2f, -FLT_MAX, +FLT_MAX, "%.0f deg");
		if (rotationChanged)
			transform.SetRotation(rotation);

		ImGui::Text("Scale:");
		glm::vec3 scale = transform.scale;
		if (ImGui::DragFloat3("Scale##scale", &scale.x, 0.05f, -FLT_MAX, +FLT_MAX, "%.3f"))
			transform.SetScale(scale);
	}


	if (ImGui::CollapsingHeader("Animate", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Rotation Speed:");
		ImGui::SliderFloat("X##rotate_speed_x", &game_object.rotate_speed_x, -90.0f, 90.0f, "%.0f deg/s");
		ImGui::SliderFloat("Y##rotate_speed_y", &game_object.rotate_speed_y, -90.0f, 90.0f, "%.0f deg/s");
		ImGui::SliderFloat("Z##rotate_speed_z", &game_object.rotate_speed_z, -90.0f, 90.0f, "%.0f deg/s");
	}

	if (ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {

		if (game_object.light) {
			ImGui::Text("Light:");
			static float light_intensity_low = 0.0f, light_intensity_high = 20.0f;
			ImGui::ColorEdit3("Color##1", (float*)&(*game_object.light).color, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Float);
			ImGui::SliderScalar("Intensity", ImGuiDataType_Float, &(*game_object.light).intensity, &light_intensity_low, &light_intensity_high, "%.1f lm");
		}

		if (game_object.pbr_sphere) {
			ImGui::Text("PBR Parameters:");
			static float pbr_roughness_low = 0.01f, pbr_roughness_high = 1.0f;
			static float pbr_metallic_low = 0.0f, pbr_metallic_high = 1.0f;
			ImGui::ColorEdit3("Albedo##pbr_albedo", (float*)&(*game_object.pbr_sphere).albedo, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Float);
			ImGui::SliderScalar("Roughness", ImGuiDataType_Float, &(*game_object.pbr_sphere).roughness, &pbr_roughness_low, &pbr_roughness_high, "%.2f");
			ImGui::SliderScalar("Metallic", ImGuiDataType_Float, &(*game_object.pbr_sphere).metallic, &pbr_metallic_low, &pbr_metallic_high, "%.2f");
		}

		if (!game_object.light && !game_object.pbr_sphere) {
			ImGui::Text("This object has no specific properties.");
		}

	}

	ImGui::End();

}


void Editor::EndFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::BeginPerformance(RenderProfiler& profiler)
{
	ImGui::Begin("Performance");

	bool enabled = profiler.IsEnabled();
	if (ImGui::Checkbox("Profiling", &enabled)) profiler.SetEnabled(enabled);
	ImGui::SameLine();
	bool paused = profiler.IsPaused();
	if (ImGui::Checkbox("Pause", &paused)) profiler.SetPaused(paused);
	ImGui::SameLine();
	if (ImGui::Button("Reset")) profiler.Reset();

	ImGui::SameLine();

	const double averageFrameMs = profiler.GetAverageFrameCpuMs();
	const double averageFps = averageFrameMs > 0.0 ? 1000.0 / averageFrameMs : 0.0;
	ImGui::Text("Frame Avg: %.2f ms  |  FPS Avg: %.1f", averageFrameMs, averageFps);

	auto stats = profiler.GetStats();
	std::stable_sort(stats.begin(), stats.end(), [](const auto& left, const auto& right) {
		if (left.averageGpuMs != right.averageGpuMs)
			return left.averageGpuMs > right.averageGpuMs;
		return left.averageCpuMs > right.averageCpuMs;
	});
	const double averageFrameGpuMs = profiler.GetAverageFrameGpuMs();

	const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
	ImGui::TextUnformatted("Frame Regions");
	if (ImGui::BeginTable("Frame regions", 5, flags, ImVec2(0.0f, 260.0f))) {
		ImGui::TableSetupColumn("Region", ImGuiTableColumnFlags_WidthStretch, 2.5f);
		ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_WidthFixed, 75.0f);
		ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_WidthFixed, 75.0f);
		ImGui::TableSetupColumn("Avg GPU", ImGuiTableColumnFlags_WidthFixed, 75.0f);
		ImGui::TableSetupColumn("GPU Avg Load", ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableHeadersRow();

		for (const auto& region : stats) {
			if (region.name.rfind("One-time/", 0) == 0) continue;
			ImGui::PushID(region.name.c_str());
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(region.name.c_str());
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("GPU min %.3f ms, max %.3f ms\n%llu CPU / %llu GPU samples",
					region.minGpuMs, region.maxGpuMs,
					static_cast<unsigned long long>(region.cpuSamples),
					static_cast<unsigned long long>(region.gpuSamples));
			}
			ImGui::TableSetColumnIndex(1);
			if (region.name == "Top-level GPU") ImGui::TextUnformatted("-");
			else ImGui::Text("%.3f ms", region.cpuMs);
			ImGui::TableSetColumnIndex(2);
			if (region.gpuSamples == 0 && region.gpuPending) ImGui::TextUnformatted("pending");
			else ImGui::Text("%.3f ms", region.gpuMs);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.3f ms", region.averageGpuMs);
			ImGui::TableSetColumnIndex(4);
			const float fraction = averageFrameGpuMs > 0.0
				? static_cast<float>(std::min(region.averageGpuMs / averageFrameGpuMs, 1.0)) : 0.0f;
			char loadLabel[16];
			snprintf(loadLabel, sizeof(loadLabel), "%.1f%%", fraction * 100.0f);
			ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0.0f), loadLabel);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	bool hasOneTimeTasks = false;
	for (const auto& region : stats) {
		if (region.name.rfind("One-time/", 0) == 0) {
			hasOneTimeTasks = true;
			break;
		}
	}

	if (hasOneTimeTasks) {
		ImGui::Spacing();
		ImGui::TextUnformatted("One-time Tasks");
		if (ImGui::BeginTable("One-time tasks", 4,
			ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthStretch, 2.5f);
			ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_WidthFixed, 85.0f);
			ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_WidthFixed, 85.0f);
			ImGui::TableSetupColumn("Avg GPU", ImGuiTableColumnFlags_WidthFixed, 85.0f);
			ImGui::TableHeadersRow();

			for (const auto& task : stats) {
				constexpr const char* prefix = "One-time/";
				if (task.name.rfind(prefix, 0) != 0) continue;
				ImGui::PushID(task.name.c_str());
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(task.name.c_str() + std::char_traits<char>::length(prefix));
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("GPU min %.3f ms, max %.3f ms\n%llu samples",
						task.minGpuMs, task.maxGpuMs,
						static_cast<unsigned long long>(task.gpuSamples));
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.3f ms", task.cpuMs);
				ImGui::TableSetColumnIndex(2);
				if (task.gpuSamples == 0 && task.gpuPending) ImGui::TextUnformatted("pending");
				else ImGui::Text("%.3f ms", task.gpuMs);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.3f ms", task.averageGpuMs);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	ImGui::End();
}

bool Editor::HasPerformanceAffectingEdit() const
{
	const ImGuiContext* context = ImGui::GetCurrentContext();
	return context != nullptr &&
		(context->ActiveIdHasBeenEditedThisFrame ||
			(context->DeactivatedItemData.ElapseFrame == context->FrameCount &&
			 context->DeactivatedItemData.HasBeenEditedBefore));
}


bool Editor::Hover() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool Editor::WantCaptureKeyboard() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}


void Editor::BeginSkyAtmosphere(SkyAtmosphere& sky) {
	SkyAtmosphereParams& p = sky.parameters;
	bool parametersChanged = false;
	ImGui::Begin("Sky Atmosphere");

	if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat("Longitude", &p.sunLongitude, 0.5f, -180.0f, 180.0f, "%.1f deg");
		parametersChanged |= ImGui::DragFloat("Latitude", &p.sunLatitude, 0.5f, -90.0f, 90.0f, "%.1f deg");
		p.sunLatitude = glm::clamp(p.sunLatitude, -90.0f, 90.0f);
		if (parametersChanged) {
			const float longitude = glm::radians(p.sunLongitude);
			const float latitude = glm::radians(p.sunLatitude);
			p.lightDirection = glm::normalize(glm::vec3(
				cos(latitude) * cos(longitude),
				sin(latitude),
				cos(latitude) * sin(longitude)));
		}
		parametersChanged |= ImGui::SliderFloat("Light Intensity", &p.lightIntensity, 0.0f, 100.0f, "%.1f");
	}

	if (ImGui::CollapsingHeader("Planet", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat("Camera Height", &p.cameraHeight, 100.0f, 1.0f, 1000000.0f, "%.0f m");
		parametersChanged |= ImGui::DragFloat("Planet Radius", &p.planetRadius, 1000.0f, 1.0f, FLT_MAX, "%.0f m");
		parametersChanged |= ImGui::DragFloat("Atmosphere Radius", &p.atmosphereRadius, 1000.0f, p.planetRadius + 1.0f, FLT_MAX, "%.0f m");
		p.atmosphereRadius = glm::max(p.atmosphereRadius, p.planetRadius + 1.0f);
	}

	if (ImGui::CollapsingHeader("Scattering", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::Checkbox("Use Rayleigh Scattering", &p.useRayleigh);
		parametersChanged |= ImGui::Checkbox("Use Mie Scattering", &p.useMie);
		parametersChanged |= ImGui::Checkbox("Use Absorption", &p.useAbsorption);
	}

	if (ImGui::CollapsingHeader("Ray Marching", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderInt("Primary Steps", &p.primarySteps, 1, 128);
		parametersChanged |= ImGui::SliderInt("Light Steps", &p.lightSteps, 1, 128);
	}
	
	if (ImGui::CollapsingHeader("Acceleration", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::Checkbox("Use Transmittance LUT", &p.useTransmittanceLUT);
		parametersChanged |= ImGui::Checkbox("Use Sky-View LUT", &p.useSkyViewLUT);
	}

	if (parametersChanged)
		sky.MarkParametersDirty();

	ImGui::End();
}

void Editor::BeginVolumetricCloud(VolumetricCloud& cloud) {
	VolumetricCloudParameters& p = cloud.parameters;

	bool parametersChanged = false;
	if (!ImGui::Begin("Volumetric Cloud")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Density", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderFloat("Density Scale", &p.densityScale, 0.0f, 10.0f, "%.2f");
		parametersChanged |= ImGui::SliderFloat("Extinction", &p.extinction, 0.0f, 10.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Cloud Map", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat("Cloud Map Scale", &p.cloudMapScale, 0.5f, 1.0f, 10000000.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Noise", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::Checkbox("Use Low Frequency Noise", &p.useLowFreqNoise);
		parametersChanged |= ImGui::DragFloat("Low Frequency Noise Scale", &p.lowFreqNoiseScale, 0.5f, 1.0f, 10000000.0f, "%.2f");

		parametersChanged |= ImGui::Checkbox("Use High Frequency Noise", &p.useHighFreqNoise);
		parametersChanged |= ImGui::DragFloat("High Frequency Noise Scale", &p.highFreqNoiseScale, 0.5f, 1.0f, 10000000.0f, "%.2f");

		parametersChanged |= ImGui::SliderFloat("Erosion Strength", &p.erosionStrength, 0.0f, 1.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat3("Light Direction", &p.lightDirection.x, 0.01f, -1.0f, 1.0f, "%.2f");
		parametersChanged |= ImGui::ColorEdit3("Light Color", &p.lightColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
		parametersChanged |= ImGui::ColorEdit3("Ambient Light", &p.ambientLight.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
		parametersChanged |= ImGui::SliderFloat("Light Intensity", &p.lightIntensity, 0.0f, 50.0f, "%.1f");
	}

	if (ImGui::CollapsingHeader("Ray Marching", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderInt("Primary Steps", &p.primarySteps, 1, 96);
		parametersChanged |= ImGui::SliderInt("Light Steps", &p.lightSteps, 1, 64);
		parametersChanged |= ImGui::SliderFloat("Ray Jitter", &p.rayJitterStrength, 0.0f, 1.0f, "%.2f");
		parametersChanged |= ImGui::DragFloat("Transmittance Cutoff", &p.transmittanceCutoff, 0.0005f, 0.0001f, 0.1f, "%.4f");
	}

	if (ImGui::CollapsingHeader("Nubis Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderFloat("Phase G", &p.phaseG, -0.95f, 0.95f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Cloud Volume", ImGuiTreeNodeFlags_DefaultOpen)) {
		glm::vec3 scale = p.cloudMapVolumeScale;
		glm::vec3 translation = p.cloudMapVolumeTranslation;
		bool transformChanged = false;
		transformChanged |= ImGui::DragFloat3("Volume Scale", &scale.x, 0.5f, 0.01f, 10000000.0f, "%.4f");
		transformChanged |= ImGui::DragFloat3("Volume Translation", &translation.x, 0.5f, -1000.0f, 10000000.0f, "%.4f");
		if (transformChanged) {
			scale = glm::max(scale, glm::vec3(0.01f));
			cloud.updateVolumeTransform(scale, translation);
			parametersChanged = true;
			p.cloudMapVolumeScale = scale;
			p.cloudMapVolumeTranslation = translation;
		}
	}

	if (parametersChanged)
		cloud.MarkParametersDirty();

	ImGui::End();
}

void Editor::BeginHierarchy(Scene& scene) {
	if (!ImGui::Begin("Scene")) {
		ImGui::End();
		return;
	}

	GameObject* selected = scene.GetSelected();
	for (const auto& object : scene.GetGameObjects()) {
		ImGui::PushID(object->GetID());
		if (ImGui::Selectable(object->GetName().c_str(), object.get() == selected)) scene.SetSelected(object->GetID());
		ImGui::PopID();
	}

	ImGui::End();
}
