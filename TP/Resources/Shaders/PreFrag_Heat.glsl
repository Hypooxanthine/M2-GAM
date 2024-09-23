// From vertex shader
// vec3 v_Normal;
// vec3 v_Position;
// vec2 v_TexCoord;
in float v_Heat;

// From application
// vec3 u_ViewPosition;
// vec3 u_LightDirection;
// vec3 u_LightColor;

// Default phong values
void PreFrag(out vec3 ambient, out vec3 diffuse, out vec3 specular, out float shininess)
{
    float normalizedHeat = v_Heat / 10.0;
    normalizedHeat = clamp(normalizedHeat, 0.0, 1.0);
    ambient = vec3(normalizedHeat, 0, 1 - normalizedHeat);
    diffuse = vec3(0);
    specular = vec3(0);
    shininess = 1.0;
}