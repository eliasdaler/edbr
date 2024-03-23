float calculateSpecularBP(float NoH) {
    float shininess = 32.0 * 4.0;
    return pow(NoH, shininess);
}

vec3 blinnPhongBRDF(vec3 diffuse, vec3 n, vec3 v, vec3 l, vec3 h) {
    vec3 Fd = diffuse;

    // specular
    // TODO: read from spec texture / pass spec param
    vec3 specularColor = diffuse * 0.5;
    float NoH = clamp(dot(n, h), 0, 1);
    vec3 Fr = specularColor * calculateSpecularBP(NoH);

    return Fd + Fr;
}

