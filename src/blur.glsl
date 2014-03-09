#if defined(VERTEX)

in vec2 VertexPosition;

out vec2 uv;

void main(void)
{   
    uv = (VertexPosition) * 0.5 + 0.5;
    gl_Position = vec4(VertexPosition.xy, 0.0, 1.0);
}

#endif

#if defined(FRAGMENT)

in vec2 uv;

uniform sampler2D Texture1;
uniform int IsBlured = 1;
uniform float Sigma = 0.5;

out vec4  OutColor;

void main ()
{
	vec3 blured = vec3(0.0);
	//~ int samples = SampleCount * 2;
	
	if(IsBlured == 1){
	
		float gaussian[25];
		
		if(Sigma > 0.9) {
		
			// sigma = 1
			gaussian[0] = 0.003765;
			gaussian[1] = 0.015019;
			gaussian[2] = 0.023792;
			gaussian[3] = 0.015019;
			gaussian[4] = 0.003765;
			
			gaussian[5] = 0.015019;
			gaussian[6] = 0.059912;
			gaussian[7] = 0.094907;
			gaussian[8] = 0.059912;
			gaussian[9] = 0.015019;
			
			gaussian[10] = 0.023792;
			gaussian[11] = 0.094907;
			gaussian[12] = 0.150342;
			gaussian[13] = 0.094907;
			gaussian[14] = 0.023792;
			
			gaussian[15] = 0.015019;
			gaussian[16] = 0.059912;
			gaussian[17] = 0.094907;
			gaussian[18] = 0.059912;
			gaussian[19] = 0.015019;
			
			gaussian[20] = 0.003765;
			gaussian[21] = 0.015019;
			gaussian[22] = 0.023792;
			gaussian[23] = 0.015019;
			gaussian[24] = 0.003765;
			
		}else{
			
			// sigma = 0.5
			gaussian[0] = 0.000002;
			gaussian[1] = 0.000212;
			gaussian[2] = 0.000922;
			gaussian[3] = 0.000212;
			gaussian[4] = 0.000002;
			
			gaussian[5] = 0.000212;
			gaussian[6] = 0.024745;
			gaussian[7] = 0.107391;
			gaussian[8] = 0.024745;
			gaussian[9] = 0.000212;
			
			gaussian[10] = 0.000922;
			gaussian[11] = 0.107391;
			gaussian[12] = 0.466066;
			gaussian[13] = 0.107391;
			gaussian[14] = 0.000922;
			
			gaussian[15] = 0.000212;
			gaussian[16] = 0.024745;
			gaussian[17] = 0.94907;
			gaussian[18] = 0.024745;
			gaussian[19] = 0.000212;
			
			gaussian[20] = 0.000002;
			gaussian[21] = 0.000212;
			gaussian[22] = 0.000922;
			gaussian[23] = 0.000212;
			gaussian[24] = 0.000002;
			
		}
		
		for(int i = -2; i <= 2; ++i){
			for(int j = -2; j <= 2; ++j){
				vec2 tempUV = uv;
				tempUV.x += i / 1024.0f;
				tempUV.y += j / 768.0f;
				
				int idx = (i+2) + (j+2)*5;
				
				blured += texture(Texture1, tempUV).rgb * gaussian[idx];
			}
		}
		
		
		//~ blured /= 273.0f;
		//~ blured /= 6.161f;
	}else{
		blured = texture(Texture1, uv).rgb;
	}
	
    OutColor = vec4(blured, 1.0);
}
#endif

