#include "Shader.h"

Vec3f normal_fragment_shader(const rst::pixel_shader_payload &payload, rst::rasterizer &ras)
{
    Vec3f result_color = (payload.normal + Vec3f{1.0f, 1.0f, 1.0f}) / 2;

    return result_color * 255;
}

Vec3f white_fragment_shader(const rst::pixel_shader_payload &payload, rst::rasterizer &ras)
{
    Vec3f white_color = Vec3f{255.f, 255.f, 255.f};

    // 获取法线和光照信息
    const Vec3f &normal = payload.normal;
    Vec3f result_color = {0.f, 0.f, 0.f};

    // 遍历所有光源
    for (const auto &light : ras.get_scene()->get_lights())
    {
        // 计算光照方向
        Vec3f light_dir = (light->position - payload.view_pos).normalize();

        // 计算漫反射强度
        float diffuse = std::max(0.f, normal * light_dir);

        // 计算光照衰减（假设光源强度随距离平方衰减）
        float r_squared = (light->position - payload.view_pos).norm() * (light->position - payload.view_pos).norm();
        Vec3f light_intensity = light->intensity / r_squared;

        // 叠加光照效果
        result_color += white_color.cwiseProduct(light_intensity) * diffuse;
    }

    // 添加环境光
    result_color += ras.get_material()->Ka.cwiseProduct(payload.amb_light_intensity);

    return result_color;
}
/*
Vec3f phong_fragment_shader(const rst::pixel_shader_payload &payload, rst::rasterizer &ras)
{
    auto material = ras.get_material();

    Vec3f ka = material->Ka;
    Vec3f kd = payload.color;
    Vec3f ks = material->Ks;

    Vec3f amb_light_intensity = payload.amb_light_intensity; // 环境光强度
    auto eye_pos = ras.get_scene()->get_camera()->eye_pos;         // 相机位置
    float p = material->specularExponent;                          // 高光指数

    Vec3f point = payload.view_pos;
    Vec3f normal = payload.normal;

    Vec3f result_color = {0, 0, 0};
    for (auto &light : ras.get_scene()->get_lights())
    {
        // 光照方向
        Vec3f light_dir = (light->position - point).normalize();
        // 视线方向
        Vec3f view_dir = (eye_pos - point).normalize();
        // 半程向量
        Vec3f half_dir = (light_dir + view_dir).normalize();

        // 光照衰减
        float r_r = (light->position - point) * (light->position - point);

        // 漫反射
        float diffuse = std::max(0.0f, normal * light_dir);

        // 镜面反射
        float specular = std::pow(std::max(0.0f, normal * half_dir), p);

        // diffuse
        Vec3f ld = kd.cwiseProduct(light->intensity / r_r) * diffuse;
        // specular
        Vec3f ls = ks.cwiseProduct(light->intensity / r_r) * specular;
        // ambient
        Vec3f la = ka.cwiseProduct(amb_light_intensity);

        result_color += (la + ld + ls);
    }

    return result_color * 255;
}
*/
Vec3f phong_fragment_shader(const rst::pixel_shader_payload &payload, rst::rasterizer &ras)
{
    auto material = ras.get_material();
    Vec3f ka = material->Ka;
    Vec3f kd = payload.color;
    Vec3f ks = material->Ks;
    Vec3f amb_light_intensity = payload.amb_light_intensity;
    auto eye_pos = ras.get_scene()->get_camera()->eye_pos;
    float p = material->specularExponent;
    Vec3f point  = payload.view_pos;
    Vec3f normal = payload.normal;

    Vec3f result_color = {0.f, 0.f, 0.f};

    for (auto &light : ras.get_scene()->get_lights())
    {
        Vec3f light_dir = (light->position - point).normalize();
        Vec3f view_dir  = (eye_pos - point).normalize();
        Vec3f half_dir  = (light_dir + view_dir).normalize();
        float r_r       = (light->position - point) * (light->position - point);
        float diffuse   = std::max(0.f, normal * light_dir);
        float specular  = std::pow(std::max(0.f, normal * half_dir), p);

        Vec3f ld = kd.cwiseProduct(light->intensity / r_r) * diffuse;
        Vec3f ls = ks.cwiseProduct(light->intensity / r_r) * specular;
        Vec3f la = ka.cwiseProduct(amb_light_intensity);

        // ========== 新增：阴影可见性计算 ==========
        float visibility = 1.f; // 默认完全可见

        if (ras.is_shadow_enabled())
        {
            // 1. 把当前点（视空间坐标）变换到光源裁剪空间
            //    point 是视空间坐标，需要先乘 view_inv 还原到世界空间
            //    但这里简化处理：直接用光源空间 MVP 变换视空间坐标
            Vec4f shadow_coord = ras.get_light_space_mvp() * point.toVector4(1.f);

            // 2. 透视除法，变换到 NDC [-1,1]
            if (std::abs(shadow_coord.w()) > 1e-5f)
                shadow_coord /= shadow_coord.w();

            // 3. NDC 转 Shadow Map 像素坐标 [0, width-1]
            int shadow_x = static_cast<int>((shadow_coord.x + 1.f) * 0.5f * (ras.get_width()  - 1));
            int shadow_y = static_cast<int>((shadow_coord.y + 1.f) * 0.5f * (ras.get_height() - 1));

            // 4. 边界检查
            if (shadow_x >= 0 && shadow_x < ras.get_width() &&
                shadow_y >= 0 && shadow_y < ras.get_height())
            {
                // 5. 查询 Shadow Map 里记录的最近深度
                int idx = (ras.get_height() - shadow_y - 1) * ras.get_width() + shadow_x;
                float d_map = ras.get_shadow_depth_buf()[idx];

                // 6. 深度比较：当前点深度 > 光源能看到的最近深度 → 在阴影里
                //    bias 防止浮点精度问题导致自遮挡（Shadow Acne）
                //    动态 bias：法线和光照方向夹角越大，bias 越大
                float cos_theta = std::max(0.f, normal * light_dir);
                float bias = std::max(0.05f * (1.f - cos_theta), 0.005f);

                if (shadow_coord.z < d_map - bias)
                    visibility = 0.3f; // 在阴影里，保留 30% 的环境光
            }
        }
        // ========== 阴影计算结束 ==========

        result_color += la + visibility * (ld + ls);
    }

    return result_color * 255.f;
}


Vec3f texture_fragment_shader(const rst::pixel_shader_payload &payload, rst::rasterizer &ras)
{
    auto material = ras.get_material();
    Vec3f texture_color = material->map_Kd.has_value() ? material->map_Kd->getColor(payload.tex_coords.x, payload.tex_coords.y) : Vec3f{0, 0, 0};

    // 材质属性
    Vec3f ka = material->Ka;          // 环境光系数
    Vec3f kd = texture_color / 255.f; // 漫反射系数
    Vec3f ks = material->Ks;          // 镜面反射系数

    // 相机属性
    Vec3f amb_light_intensity = payload.amb_light_intensity;      // 环境光强度
    auto eye_pos = ras.get_scene()->get_camera()->eye_pos;                // 相机位置
    float p = material->specularExponent; // 高光指数

    Vec3f point = payload.view_pos;
    Vec3f normal = payload.normal;

    Vec3f result_color = {0, 0, 0};

    for (auto &light : ras.get_scene()->get_lights())
    {
        // 光照方向
        Vec3f light_dir = (light->position - point).normalize();
        // 视线方向
        Vec3f view_dir = (eye_pos - point).normalize();
        // 半程向量
        Vec3f half_dir = (light_dir + view_dir).normalize();

        // 光照衰减
        float r_r = (light->position - point) * (light->position - point);

        // 漫反射
        float diffuse = std::max(0.0f, normal * light_dir);

        // 镜面反射
        float specular = std::pow(std::max(0.0f, normal * half_dir), p);

        // diffuse
        Vec3f ld = kd.cwiseProduct(light->intensity / r_r) * diffuse;
        // specular
        Vec3f ls = ks.cwiseProduct(light->intensity / r_r) * specular;
        // ambient
        Vec3f la = ka.cwiseProduct(amb_light_intensity);

        result_color += (la + ld + ls);
    }

    return result_color * 255;
}

Vec3f bump_fragment_shader(const rst::pixel_shader_payload &payload, rst::rasterizer &ras)
{
    auto material = ras.get_material();
    Vec3f normal = payload.normal;

    float kh = 0.2f, kn = 0.1f; // kh 和 kn 是控制凹凸效果的参数

    // 用于从纹理中获取颜色并计算其亮度（norm）
    auto texture_intensity = [&payload, &material](const float u, const float v)
    {
        return material->map_bump.has_value() ? material->map_bump->getColor(u, v).norm() : 1.f;
    };

    // 获取当前法线的 x, y, z 分量
    float x = normal.x;
    float y = normal.y;
    float z = normal.z;

    // 获取纹理坐标 u, v
    float u = payload.tex_coords.x;
    float v = payload.tex_coords.y;

    // 获取凹凸贴图的宽度和高度
    auto w = material->map_Kd->getWidth();
    auto h = material->map_Kd->getHeight();

    // 计算切线向量 tangent
    float sqrt_xz = sqrt(x * x + z * z);
    auto tangent = Vec3f{x * y / sqrt_xz, - sqrt_xz, z * y / sqrt_xz};

    // 计算副切线向量 bitangent
    auto bitangent = (normal ^ tangent).normalize();

    // 构建 TBN 矩阵，TBN 矩阵用于将局部坐标系中的向量转换到世界坐标系
    Matrix3f TBN{{tangent, bitangent, normal}};

    // 计算 u 方向的梯度 du，通过纹理坐标的微小变化来计算高度变化
    float du = kh * kn * (texture_intensity(u + 1.f / w, v) - texture_intensity(u, v));

    // 计算 v 方向的梯度 dv
    float dv = kh * kn * (texture_intensity(u, v + 1.f / h) - texture_intensity(u, v));

    // 计算局部法线 ln，根据梯度 du 和 dv 调整法线
    auto ln = Vec3f{-du, -dv, 1.f};
    normal = (TBN * ln).normalize();

    // 返回扰动后的法线向量
    return normal * 255;
}

Vec3f displacement_fragment_shader(const rst::pixel_shader_payload &payload, rst::rasterizer &ras)
{
    auto material = ras.get_material();
    Vec3f bump_color = material->map_bump.has_value() ? material->map_bump->getColor(payload.tex_coords.x, payload.tex_coords.y) : Vec3f{0, 0, 0};

    Vec3f ka = material->Ka;
    Vec3f kd = bump_color / 255;
    Vec3f ks = material->Ks;

    // 相机属性
    Vec3f amb_light_intensity = payload.amb_light_intensity; // 环境光强度
    auto eye_pos = ras.get_scene()->get_camera()->eye_pos;   // 相机位置
    float p = material->specularExponent;             // 高光指数

    Vec3f point = payload.view_pos;
    Vec3f normal = payload.normal;

    float kh = 0.2f, kn = 0.1f; // kh 和 kn 是控制凹凸效果的参数

    auto texture_intensity = [&payload, &material](const float u, const float v)
    {
        return material->map_bump.has_value() ? material->map_bump->getColor(u, v).norm() : 1.f;
    };

    // 获取当前法线的 x, y, z 分量
    float x = normal.x;
    float y = normal.y;
    float z = normal.z;

    // 获取纹理坐标 u, v
    float u = payload.tex_coords.x;
    float v = payload.tex_coords.y;

    // 获取纹理的宽度和高度
    auto w = material->map_Kd->getWidth();
    auto h = material->map_Kd->getHeight();

    // 计算切线向量 tangent
    float sqrt_xz = sqrt(x * x + z * z);
    auto tangent = Vec3f{x * y / sqrt_xz, - sqrt_xz, z * y / sqrt_xz};

    // 计算副切线向量 bitangent，通过法线和切线的叉积得到
    auto bitangent = (normal ^ tangent).normalize();

    // 构建 TBN 矩阵，TBN 矩阵用于将局部坐标系中的向量转换到世界坐标系
    Matrix3f TBN{{tangent, bitangent, normal}};

    // 计算 u 方向的梯度 du，通过纹理坐标的微小变化来计算高度变化
    float du = kh * kn * (texture_intensity(u + 1.f / w, v) - texture_intensity(u, v));

    // 计算 v 方向的梯度 dv
    float dv = kh * kn * (texture_intensity(u, v + 1.f / h) - texture_intensity(u, v));

    // 计算局部法线 ln，根据梯度 du 和 dv 调整法线
    auto ln = Vec3f{-du, -dv, 1.f};
    normal = (TBN * ln).normalize();

    // 移动顶点高度
    point += kn * normal * texture_intensity(u, v);

    Vec3f result_color = {0, 0, 0};

    for (auto &light : ras.get_scene()->get_lights())
    {
        // 光照方向
        Vec3f light_dir = (light->position - point).normalize();
        // 视线方向
        Vec3f view_dir = (eye_pos - point).normalize();
        // 半程向量
        Vec3f half_dir = (light_dir + view_dir).normalize();

        // 光照衰减
        float r_r = (light->position - point) * (light->position - point);

        // 漫反射
        float diffuse = std::max(0.0f, normal * light_dir);

        // 镜面反射
        float specular = std::pow(std::max(0.0f, normal * half_dir), p);

        // diffuse
        Vec3f ld = kd.cwiseProduct(light->intensity / r_r) * diffuse;
        // specular
        Vec3f ls = ks.cwiseProduct(light->intensity / r_r) * specular;
        // ambient
        Vec3f la = ka.cwiseProduct(amb_light_intensity);

        result_color += (la + ld + ls);
    }

    return result_color * 255;
}

void vertex_shader(rst::vertex_shader_payload &payload, Triangle *t)
{
    auto t_Vec4 = t->toVector4(t->get_vertex(), 1.f);

    auto &viewspace_pos = t->get_viewspace_pos();
    auto &normal = t->get_normal();

    viewspace_pos = {
        (payload.view * payload.model * t_Vec4[0]).head<3>(),
        (payload.view * payload.model * t_Vec4[1]).head<3>(),
        (payload.view * payload.model * t_Vec4[2]).head<3>()};

    normal = {
        (payload.inv_trans * normal[0].toVector4(0.0f)).head<3>(),
        (payload.inv_trans * normal[1].toVector4(0.0f)).head<3>(),
        (payload.inv_trans * normal[2].toVector4(0.0f)).head<3>()};

    for (int i = 0; i < 3; ++i)
    {
        // 将顶点从模型空间转换到裁剪空间（NDC）
        Vec4f v = payload.mvp * t_Vec4[i];
        v /= v.w();

        t->setVertex(i, v.head<3>());
    }
}