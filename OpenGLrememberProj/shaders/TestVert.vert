uniform vec3 aPos;
 
void main()
{
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix*gl_Vertex;
	Position = gl_Vertex.xyz;
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}