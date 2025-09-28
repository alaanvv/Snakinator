#version 330 core

// --- Struct

struct Material {
  vec3 COL;
  sampler2D S_DIF, S_EMT;
  float AMB, DIF;
  int LIG;
};

struct DirLig {
  vec3 COL, DIR;
};

struct PntLig {
  vec3  COL, POS;
  float CON, LIN, QUA;
};

struct SptLig {
  vec3  COL, POS, DIR;
  float CON, LIN, QUA, INN, OUT;
};

// --- Setup

uniform vec2 TEX_SCALE;
uniform Material MAT;
uniform DirLig DIR_LIGS[10];
uniform PntLig PNT_LIGS[10];
uniform SptLig SPT_LIGS[10];

uniform int DIR_LIG_AMOUNT;
uniform int PNT_LIG_AMOUNT;
uniform int SPT_LIG_AMOUNT;

in  vec3 nrm;
in  vec3 pos;
in  vec2 tex;
out vec4 color;

// --- Function

vec3 CalcDirLig(DirLig lig, vec3 normal) {
  vec3 light_dir = normalize(-lig.DIR);

  vec3 ambient = lig.COL * MAT.COL * MAT.AMB;
  ambient *= vec3(texture(MAT.S_DIF, tex));
  ambient += vec3(texture(MAT.S_EMT, tex));

  vec3 diffuse = lig.COL * MAT.COL * MAT.DIF * max(dot(normal, light_dir), 0);
  diffuse *= vec3(texture(MAT.S_DIF, tex));

  return ambient + diffuse;
}

vec3 CalcPntLig(PntLig lig, vec3 normal) {
  vec3 light_dir = normalize(lig.POS - pos);

  float distance = length(lig.POS - pos);
  float attenuation = 1 / (lig.CON + lig.LIN * distance + lig.QUA * distance * distance);

  vec3 ambient = attenuation * lig.COL * MAT.COL * MAT.AMB;
  ambient *= vec3(texture(MAT.S_DIF, tex));
  ambient += vec3(texture(MAT.S_EMT, tex));

  vec3 diffuse = attenuation * lig.COL * MAT.COL * MAT.DIF * max(dot(normalize(normal), light_dir), 0);
  diffuse *= vec3(texture(MAT.S_DIF, tex));

  return ambient + diffuse;
}

vec3 CalcSptLig(SptLig lig, vec3 normal) {
  vec3 light_dir = normalize(lig.POS - pos);

  float theta = dot(light_dir, normalize(-lig.DIR));
  float epsilon = lig.INN - lig.OUT;
  float intensity = clamp((theta - lig.OUT) / epsilon, 0, 1);

  float distance = length(lig.POS - pos);
  float attenuation = 1 / (lig.CON + lig.LIN * distance + lig.QUA * distance * distance);

  vec3 ambient = attenuation * lig.COL * MAT.COL * MAT.AMB;
  ambient *= vec3(texture(MAT.S_DIF, tex));
  ambient += vec3(texture(MAT.S_EMT, tex));

  vec3 diffuse = intensity * attenuation * lig.COL * MAT.COL * MAT.DIF * max(dot(normalize(normal), light_dir), 0);
  diffuse *= vec3(texture(MAT.S_DIF, tex));

  return ambient + diffuse;
}

// --- Main

void main() {
  if (vec3(texture(MAT.S_DIF, tex)) == vec3(0, 1, 0)) {
    discard;
  }

  vec3 _color = vec3(0);

  if (MAT.LIG == 0) {
    if (DIR_LIG_AMOUNT > 0)
      for (int i = 0; i < DIR_LIG_AMOUNT; i++)
        _color += CalcDirLig(DIR_LIGS[i], nrm);

    if (PNT_LIG_AMOUNT > 0)
      for (int i = 0; i < PNT_LIG_AMOUNT; i++)
        _color += CalcPntLig(PNT_LIGS[i], nrm);

    if (SPT_LIG_AMOUNT > 0)
      for (int i = 0; i < SPT_LIG_AMOUNT; i++)
        _color += CalcSptLig(SPT_LIGS[i], nrm);
  }
  else {
    _color = MAT.COL;
  }

  if (nrm.y < -0.5 || nrm.x > 0.5 || nrm.z > 0.5) {
    _color *= 0.6;
  }

  color = vec4(_color, 1);
}
