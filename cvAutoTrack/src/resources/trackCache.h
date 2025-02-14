﻿#pragma once
//std
#include <string>
#include <fstream>
#include <vector>
#include <map>
//cv
#include <opencv2/opencv.hpp>
//cereal序列化类
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>

namespace trackCache
{
    //基础设置结构体
    struct Setting
    {
        std::string version = "1.0.0";
        int32_t octave_layers = 1;
        float_t hessian_threshold = 1.0;
        bool extended = false;
        bool up_right = false;
        //序列化模板
        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(version, extended, up_right, octave_layers, hessian_threshold);
        }
    };

    //标签信息结构体
    struct TagInfo
    {
        int id = 0;
        int parent = -1;
        float zoom = 1.0;
        std::string name;
        std::string value;
        //序列化模板
        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(id, parent, zoom, name, value);
        }
    };


    //缓存文件结构
    struct CacheFile
    {
        Setting setting;
        std::map<int, TagInfo> tag_info_map;
        std::vector<cv::KeyPoint> key_points;
        cv::Mat descriptors;
    private:
        //特征点容器定义，压缩特征点数据
        struct _Sample_KeyPoint
        {
            int16_t x;
            int16_t y;
            uint16_t response;
            int32_t class_id;
            //序列化模板
            template <class Archive>
            void serialize(Archive& ar)
            {
                ar(x, y, response, class_id);
            }
        };

    public:
        //序列化模板
        template <class Archive>
        void save(Archive& ar) const
        {
            //序列化OpenCV的数据
            //优化特征点数据
            std::vector<_Sample_KeyPoint> sample_key_points;
            for (const auto& kp : key_points)
            {
                _Sample_KeyPoint sample_kp;
                sample_kp.x = static_cast<int16_t>(kp.pt.x);
                sample_kp.y = static_cast<int16_t>(kp.pt.y);
                sample_kp.response = static_cast<uint16_t>(kp.response > UINT16_MAX ? UINT16_MAX : kp.response);
                sample_kp.class_id = kp.class_id;
                sample_key_points.emplace_back(sample_kp);
            }

            //优化特征矩阵数据，转换为半精度，并使用char数组保存
            cv::Mat descriptors_half;
            descriptors.convertTo(descriptors_half, CV_16F);
            std::vector<char> descriptors_vector(descriptors_half.data,
                descriptors_half.data + descriptors_half.total() * 2);
            //矩阵shape
            int descriptor_shape[2] = { descriptors.rows, descriptors.cols };
            //序列化数据
            ar(setting, tag_info_map, sample_key_points, descriptor_shape, descriptors_vector);
        }

        //反序列化模板
        template <class Archive>
        void load(Archive& ar)
        {
            //准备反序列化的中间容器
            std::vector<_Sample_KeyPoint> sample_key_points; //压缩后的特征点数据
            std::vector<char> descriptors_vector; //压缩后的特征矩阵数据
            int descriptor_shape[2]; //矩阵尺寸数据
            //反序列化数据
            ar(setting, tag_info_map, sample_key_points, descriptor_shape, descriptors_vector);
            //解压特征点
            key_points.clear();
            for (const auto& sample_kp : sample_key_points)
            {
                cv::KeyPoint kp;
                kp.pt.x = sample_kp.x;
                kp.pt.y = sample_kp.y;
                kp.response = sample_kp.response;
                kp.angle = 270.0;
                kp.octave = 0;
                kp.class_id = sample_kp.class_id;
                key_points.emplace_back(kp);
            }
            //解压特征矩阵
            auto descriptors_half = cv::Mat(descriptor_shape[0], descriptor_shape[1], CV_16F);
            memcpy(descriptors_half.data, descriptors_vector.data(), descriptors_vector.size());
            descriptors_half.convertTo(descriptors, CV_32F);
        }
    };

    inline CacheFile Deserialize(const std::string& fileName)
    {
        CacheFile cache_file_struct;
        std::ifstream ifs(fileName, std::ios::binary);
        cereal::BinaryInputArchive archive(ifs);
        archive(cache_file_struct);
        ifs.close();
        return cache_file_struct;
    }

    inline void Serialize(const std::string& fileName, CacheFile cacheFileStruct)
    {
        std::ofstream ofs(fileName, std::ios::binary);
        cereal::BinaryOutputArchive archive(ofs);
        archive(cacheFileStruct);
        ofs.close();
    }
}
