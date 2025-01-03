//
// Created by Monika on 23.08.2022.
//

#ifndef SR_ENGINE_TRANSFORMZERO_H
#define SR_ENGINE_TRANSFORMZERO_H

#include <Utils/ECS/Transform.h>

namespace SR_UTILS_NS {
    class GameObject;

    class SR_DLL_EXPORT TransformZero : public Transform {
        friend class GameObject;
    public:
        ~TransformZero() override = default;

    public:
        SR_NODISCARD Measurement GetMeasurement() const override { return Measurement::SpaceZero; }

        SR_NODISCARD Transform::Ptr Copy() const override;

    };

    class SR_DLL_EXPORT TransformHolder : public Transform {
        friend class GameObject;
    public:
        ~TransformHolder() override = default;

    public:
        SR_NODISCARD Measurement GetMeasurement() const override { return Measurement::SpaceZero; }

        SR_NODISCARD Transform::Ptr Copy() const override;

    };
}


#endif //SR_ENGINE_TRANSFORMZERO_H
