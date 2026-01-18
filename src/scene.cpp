
#include "Scene.h"
#include "QuadModel.h"
#include "OBJModel.h"
#include "Cube.h"

Scene::Scene(
	ID3D11Device* dxdevice,
	ID3D11DeviceContext* dxdevice_context,
	int window_width,
	int window_height) :
	m_dxdevice(dxdevice),
	m_dxdevice_context(dxdevice_context),
	m_window_width(window_width),
	m_window_height(window_height)
{ }

void Scene::OnWindowResized(
	int new_width,
	int new_height)
{
	m_window_width = new_width;
	m_window_height = new_height;
}

OurTestScene::OurTestScene(
	ID3D11Device* dxdevice,
	ID3D11DeviceContext* dxdevice_context,
	int window_width,
	int window_height) :
	Scene(dxdevice, dxdevice_context, window_width, window_height)
{ 
	InitTransformationBuffer();
	// + init other CBuffers
}

//
// Called once at initialization
//
void OurTestScene::Init()
{
	m_camera = new Camera(
		45.0f * fTO_RAD,		// field-of-view (radians)
		(float)m_window_width / m_window_height,	// aspect ratio
		1.0f,					// z-near plane (everything closer will be clipped/removed)
		500.0f);				// z-far plane (everything further will be clipped/removed)

	// Move camera to (0,0,5)
	m_camera->MoveTo({ 0, 0, 5 });

	// Create objects
	m_cube = new Cube(m_dxdevice, m_dxdevice_context);
	n_cube = new Cube(m_dxdevice, m_dxdevice_context);
	v_cube = new Cube(m_dxdevice, m_dxdevice_context);
	m_quad = new QuadModel(m_dxdevice, m_dxdevice_context);
	m_sponza = new OBJModel("assets/crytek-sponza/sponza.obj", m_dxdevice, m_dxdevice_context);
}

//
// Called every frame
// dt (seconds) is time elapsed since the previous frame
//
void OurTestScene::Update(
	float dt,
	const InputHandler& input_handler)
{
	// Basic camera control
	if (input_handler.IsKeyPressed(Keys::Up) || input_handler.IsKeyPressed(Keys::W))
		m_camera->Move({ 0.0f, 0.0f, -m_camera_velocity * dt });
	if (input_handler.IsKeyPressed(Keys::Down) || input_handler.IsKeyPressed(Keys::S))
		m_camera->Move({ 0.0f, 0.0f, m_camera_velocity * dt });
	if (input_handler.IsKeyPressed(Keys::Right) || input_handler.IsKeyPressed(Keys::D))
		m_camera->Move({ m_camera_velocity * dt, 0.0f, 0.0f });
	if (input_handler.IsKeyPressed(Keys::Left) || input_handler.IsKeyPressed(Keys::A))
		m_camera->Move({ -m_camera_velocity * dt, 0.0f, 0.0f });
	if(input_handler.IsKeyPressed(Keys::Space))
		m_camera->Move({ 0.0f, m_camera_velocity * dt, 0.0f });
	if(input_handler.IsKeyPressed(Keys::LCtrl))
		m_camera->Move({ 0.0f, -m_camera_velocity * dt, 0.0f });

	long mouse_dx = input_handler.GetMouseDeltaX();
	long mouse_dy = input_handler.GetMouseDeltaY();

	if (mouse_dx != 0 || mouse_dy != 0) {
		m_camera->Rotate(static_cast<float>(mouse_dx), static_cast<float>(mouse_dy));
	}
	
	if(input_handler.IsKeyPressed(Keys::Esc))
		PostQuitMessage(0);

	// Now set/update object transformations
	// This can be done using any sequence of transformation matrices,
	// but the T*R*S order is most common; i.e. scale, then rotate, and then translate.
	// If no transformation is desired, an identity matrix can be obtained 
	// via e.g. Mquad = linalg::mat4f_identity; 

	// Update the rotation and orbit angles
	m_cube_rotation_angle += dt * 0.5f;      // Sun rotation
	n_cube_orbit_angle += dt * 1.0f;         // Earth orbit around Sun
	n_cube_rotation_angle += dt * 3.0f;      // Earth rotation
	v_cube_orbit_angle += dt * 5.0f;         // Moon orbit around Earth (faster)
	v_cube_rotation_angle += dt * 0.5f;      // Moon rotation (slower)

	//m_cube stays at center but rotates
	m_cube_transform = mat4f::scaling(1.5f) * mat4f::rotation(m_cube_rotation_angle, 0.0f, 1.0f, 0.0f) * mat4f::translation(0, 0, 0);

	// n_cube (Earth) orbits around Sun
	float n_orbitX = n_orbit_radius * cos(n_cube_orbit_angle);
	float n_orbitZ = n_orbit_radius * sin(n_cube_orbit_angle);

	n_cube_transform = mat4f::scaling(0.7f) *
		mat4f::rotation(n_cube_rotation_angle, 0.0f, 1.0f, 0.0f) *
		mat4f::translation(n_orbitX, 0, n_orbitZ);

	// Earth's world transform (relative to Sun)
	mat4f n_cube_world_transform = m_cube_transform * n_cube_transform;

	// v_cube (Moon) orbits around Earth
	float v_orbitX = v_orbit_radius * cos(v_cube_orbit_angle);
	float v_orbitZ = v_orbit_radius * sin(v_cube_orbit_angle);

	v_cube_transform = mat4f::scaling(0.4f) *  // Moon is smallest
		mat4f::rotation(v_cube_rotation_angle, 0.0f, 1.0f, 0.0f) *
		mat4f::translation(v_orbitX, 0, v_orbitZ);



	// Quad model-to-world transformation
	m_quad_transform = mat4f::translation(0, 0, 0) *			// No translation
		mat4f::rotation(-m_angle, 0.0f, 1.0f, 0.0f) *	// Rotate continuously around the y-axis
		mat4f::scaling(1.5, 1.5, 1.5);				// Scale uniformly to 150%

	// Sponza model-to-world transformation
	m_sponza_transform = mat4f::translation(0, -5, 0) *		 // Move down 5 units
		mat4f::rotation(fPI / 2, 0.0f, 1.0f, 0.0f) * // Rotate pi/2 radians (90 degrees) around y
		mat4f::scaling(0.05f);						 // The scene is quite large so scale it down to 5%

	// Increment the rotation angle.
	m_angle += m_angular_velocity * dt;

	// Print fps
	m_fps_cooldown -= dt;
	if (m_fps_cooldown < 0.0)
	{
		std::cout << "fps " << (int)(1.0f / dt) << std::endl;
//		printf("fps %i\n", (int)(1.0f / dt));
		m_fps_cooldown = 2.0;
	}
}

//
// Called every frame, after update
//
void OurTestScene::Render()
{
	// Bind transformation_buffer to slot b0 of the VS
	m_dxdevice_context->VSSetConstantBuffers(0, 1, &m_transformation_buffer);

	// Obtain the matrices needed for rendering from the camera
	m_view_matrix = m_camera->WorldToViewMatrix();
	m_projection_matrix = m_camera->ProjectionMatrix();

	// Load matrices + the Quad's transformation to the device and render it
	UpdateTransformationBuffer(m_quad_transform, m_view_matrix, m_projection_matrix);
	m_quad->Render();

	// Render Sun (m_cube)
	UpdateTransformationBuffer(m_cube_transform, m_view_matrix, m_projection_matrix);
	m_cube->Render();

	mat4f n_cube_world_transform = m_cube_transform * n_cube_transform;


	// Render Earth (n_cube) - relative to Sun
	UpdateTransformationBuffer(n_cube_world_transform, m_view_matrix, m_projection_matrix);
	n_cube->Render();


	// Render Moon (v_cube) - relative to Earth (which is already relative to Sun)
	// Moon's world transform = Earth's world transform * Moon's local transform
	mat4f v_cube_world_transform = n_cube_world_transform * v_cube_transform;
	UpdateTransformationBuffer(v_cube_world_transform, m_view_matrix, m_projection_matrix);
	v_cube->Render();

	// Load matrices + Sponza's transformation to the device and render it
	UpdateTransformationBuffer(m_sponza_transform, m_view_matrix, m_projection_matrix);
	m_sponza->Render();
}

void OurTestScene::Release()
{
	SAFE_DELETE(m_quad);
	SAFE_DELETE(m_cube);
	SAFE_DELETE(m_sponza);
	SAFE_DELETE(m_camera);
	

	SAFE_RELEASE(m_transformation_buffer);
	// + release other CBuffers
}

void OurTestScene::OnWindowResized(
	int new_width,
	int new_height)
{
	if (m_camera)
		m_camera->SetAspect(float(new_width) / new_height);

	Scene::OnWindowResized(new_width, new_height);
}

void OurTestScene::InitTransformationBuffer()
{
	HRESULT hr;
	D3D11_BUFFER_DESC matrixBufferDesc = { 0 };
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(TransformationBuffer);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	ASSERT(hr = m_dxdevice->CreateBuffer(&matrixBufferDesc, nullptr, &m_transformation_buffer));
}

void OurTestScene::UpdateTransformationBuffer(
	mat4f ModelToWorldMatrix,
	mat4f WorldToViewMatrix,
	mat4f ProjectionMatrix)
{
	// Map the resource buffer, obtain a pointer and then write our matrices to it
	D3D11_MAPPED_SUBRESOURCE resource;
	m_dxdevice_context->Map(m_transformation_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	TransformationBuffer* matrixBuffer = (TransformationBuffer*)resource.pData;
	matrixBuffer->ModelToWorldMatrix = ModelToWorldMatrix;
	matrixBuffer->WorldToViewMatrix = WorldToViewMatrix;
	matrixBuffer->ProjectionMatrix = ProjectionMatrix;
	m_dxdevice_context->Unmap(m_transformation_buffer, 0);
}