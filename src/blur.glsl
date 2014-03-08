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

out vec4  OutColor;

void main ()
{
	vec4 blured = vec4(0.0);
	//~ int samples = SampleCount * 2;
	
	if(IsBlured == 1){
	
		float gaussian[25];
		gaussian[0] = 1.0;
		gaussian[1] = 4.0;
		gaussian[2] = 7.0;
		gaussian[3] = 4.0;
		gaussian[4] = 1.0;
		
		gaussian[5] = 4.0;
		gaussian[6] = 16.0;
		gaussian[7] = 26.0;
		gaussian[8] = 16.0;
		gaussian[9] = 4.0;
		
		gaussian[10] = 7.0;
		gaussian[11] = 26.0;
		gaussian[12] = 41.0;
		gaussian[13] = 26.0;
		gaussian[14] = 7.0;
		
		gaussian[15] = 4.0;
		gaussian[16] = 16.0;
		gaussian[17] = 26.0;
		gaussian[18] = 16.0;
		gaussian[19] = 4.0;
		
		gaussian[20] = 1.0;
		gaussian[21] = 4.0;
		gaussian[22] = 7.0;
		gaussian[23] = 4.0;
		gaussian[24] = 1.0;
		
		
		for(int i = -2; i <= 2; ++i){
			for(int j = -2; j <= 2; ++j){
				vec2 tempUV = uv;
				tempUV.x += i / 1024.0f;
				tempUV.y += j / 768.0f;
				
				int idx = (i+2) + (j+2)*5;
				
				blured += texture(Texture1, tempUV).rgba * gaussian[idx];
			}
		}
		
		blured /= 273.0f;
	}else{
		blured = texture(Texture1, uv).rgba;
	}
	
    OutColor = blured;
}
#endif

