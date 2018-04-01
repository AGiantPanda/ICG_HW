// my shader
    precision mediump float;

    /*some uniform parameters*/ 
    varying vec2 fragCoord;
    uniform vec3 iResolution;  		// Viewport resolution in pixels;
    uniform vec3 iCameraPos;		// CameraPosition in world space;
    uniform vec3 iCameraDir;		// CameraDirection in world space, camera up would always be (0.0, 1.0, 0.0);
    uniform float iGlobalTime;		// Total redering time;
    //uniform float iOctaves;			// Used for octave iteration
    uniform sampler2D uSampler0;	// First image sampler

    const int MAX_MARCHING_STEPS = 100;
    const float MIN_DIST = 0.0;
    const float MAX_DIST = 100.0;
    const float EPSILON = 0.001;

    vec3 calcRayDirection(float fieldofView, vec2 resolution, vec2 fragCoord)
    {
    	vec2 xy = fragCoord * resolution - resolution * 0.5;
    	float z = resolution.y / tan(radians(fieldofView) / 2.0);
    	return normalize(vec3(xy, -z));
    }

    // a noise function from iq
    float Noise(vec3 x)
    {
    	vec3 p = floor(x);
    	vec3 f = fract(x);
    	f = f * f * (3.0 - 2.0 * f);

		vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
		vec2 rg = texture2D( uSampler0, (uv+0.5)/256.0, -100.0 ).yx;
		return mix( rg.x, rg.y, f.z );
    }

    // generate the height of the wave at "pos" position
    float Waves(vec3 pos)
    {
    	pos *= 0.2 * vec3(1, 1, 1);

    	const int octaves = 8;
    	float amp = 1.5;
    	float h = 0.0;

    	pos += iGlobalTime * vec3(0.1, 0.1, 0.1);
    	for(int i = 0; i< octaves; i++)
    	{
    		h += amp * abs(Noise(pos) - 0.5);
    		pos *= 2.0;	// double the frequency
    		amp *= 0.5; // half the amplitude
    	}

    	return -h;
    }

    // return the height distance between pos to the ocean
    float getOceanDistance(vec3 pos)
    {
    	return pos.y - Waves(pos);
    }

    vec3 getOceanNormal(vec3 pos)
    {
    	float offset = 10.0 * EPSILON * length(pos);
    	return normalize(vec3(
    		getOceanDistance(vec3(pos.x + offset, pos.y, pos.z)) - getOceanDistance(vec3(pos.x - offset, pos.y, pos.z)),
    		getOceanDistance(vec3(pos.x, pos.y + offset, pos.z)) - getOceanDistance(vec3(pos.x, pos.y - offset, pos.z)),
    		getOceanDistance(vec3(pos.x, pos.y, pos.z + offset)) - getOceanDistance(vec3(pos.x, pos.y, pos.z - offset))
    		));
    }

    float traceOcean(vec3 ro, vec3 rd)
    {
    	float t = MIN_DIST;
    	for(int i = 0; i < MAX_MARCHING_STEPS; i++)
    	{
    		// use height instead of distance to a specific point, it can accelerate the whole algorithm
    		float h = getOceanDistance(ro + t * rd);
    		if(h < EPSILON)
    			return t;
    		t += h;
    	}
    	return MIN_DIST;
    }

    vec3 shadeSky(vec3 rd)
    {
    	return vec3(.4, .45, .5);
    }

    vec3 shadeOcean(vec3 ro, vec3 rd)
    {
    	vec3 norm = getOceanNormal(ro);

    	vec3 reflectedRay = normalize(reflect(rd, norm));

    	// mix 
    	float fresnel = pow(1.0 - abs(dot(rd, norm)), 5.0);

    	vec3 reflection = shadeSky(ro);
    	vec3 col = vec3(0, .04, .04);
    	col = mix(col, reflection, fresnel);

    	return col;
    }

    void main(void) {
    	vec3 ro = iCameraPos;
    	vec3 rd = calcRayDirection();

    	float t = traceOcean(ro, rd);

    	vec3 col;

    	if(t > 0.0)
    		col = shadeOcean(ro + t * rd, rd);
    	else
    		col = shadeSky(ro, rd);

        gl_FragColor = vec4(col, 1.0);
    }