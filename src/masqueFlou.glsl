#if defined(VERTEX)

in vec2 VertexPosition;

out vec2 uv;

void main(void)
{   
	uv = VertexPosition * 0.5 + 0.5;
    gl_Position = vec4(VertexPosition.xy, 0.0, 1.0);
}

#endif

#if defined(FRAGMENT)

in vec2 uv;

uniform sampler2D BluredImage;
uniform sampler2D MidDetails;
uniform sampler2D SmallDetails;

uniform float BluredCoef = 1.0;
uniform float MidCoef = 1.0;
uniform float SmallCoef = 1.0;

out vec4 OutColor;

void main(void)
{
	vec3 blured = texture(BluredImage, uv).rgb;
	vec3 midDetails = texture(MidDetails, uv).rgb;
	vec3 smallDetails = texture(SmallDetails, uv).rgb;
	
	vec3 color = blured * BluredCoef + midDetails * MidCoef + smallDetails * SmallCoef;
	
    OutColor = vec4(color,1.0);
}

#endif

