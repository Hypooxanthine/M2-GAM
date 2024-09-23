// From vertex shader
// vec3 v_Normal;
// vec3 v_Position;
// vec2 v_TexCoord;
in float v_Curvature;

// From application
// vec3 u_ViewPosition;
// vec3 u_LightDirection;
// vec3 u_LightColor;

// Default phong values
void PreFrag(out vec3 ambient, out vec3 diffuse, out vec3 specular, out float shininess)
{
    float normalizedCurvature = v_Curvature / 100;
    ambient = vec3(normalizedCurvature, 0, 1 - normalizedCurvature);
    diffuse = vec3(0);
    specular = vec3(0);
    shininess = 1.0;
}