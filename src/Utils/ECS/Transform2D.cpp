//
// Created by Monika on 01.08.2022.
//

#include <Utils/ECS/Transform2D.h>

namespace SR_UTILS_NS {
    Transform2D::Transform2D()
        : Transform()
    { }

    void Transform2D::SetTranslation(const SR_MATH_NS::FVector3& translation) {
        if (translation == m_translation) {
            return;
        }

        m_translation = translation;

        UpdateTree();
    }

    void Transform2D::SetTranslationAndRotation(const SR_MATH_NS::FVector3& translation, const SR_MATH_NS::FVector3& euler) {
        m_translation = translation;
        m_rotation = euler.Limits(360);

        UpdateTree();
    }

    void Transform2D::SetRotation(const SR_MATH_NS::FVector3& euler) {
        m_rotation = euler.Limits(360);

        UpdateTree();
    }

    void Transform2D::SetScale(const SR_MATH_NS::FVector3& rawScale) {
        SR_MATH_NS::FVector3 scale = rawScale;

        if (scale.ContainsNaN()) {
            SR_WARN("Transform2D::SetScale() : scale contains NaN! Reset...");
            scale = SR_MATH_NS::FVector3::One();
        }

        m_scale = scale;

        UpdateTree();
    }

    void Transform2D::SetSkew(const SR_MATH_NS::FVector3& rawSkew) {
        SR_MATH_NS::FVector3 skew = rawSkew;

        if (skew.ContainsNaN()) {
            SR_WARN("Transform2D::SetSkew() : skew contains NaN! Reset...");
            skew = Math::FVector3::One();
        }

        m_skew = skew;

        UpdateTree();
    }

    const SR_MATH_NS::Matrix4x4& Transform2D::GetMatrix() {
        if (IsDirty()) {
            UpdateMatrix();

            if (auto&& pTransform = m_gameObject->GetParentTransform()) {
                m_matrix = pTransform->GetMatrix() * m_localMatrix;
            }
            else {
                m_matrix = m_localMatrix;
            }
        }

        return m_matrix;
    }

    void Transform2D::UpdateMatrix() {
        auto&& scale = CalculateStretch();
        auto&& translation = CalculateAnchor(scale);

        m_localMatrix = SR_MATH_NS::Matrix4x4(
            translation,
            m_rotation.Radians().ToQuat(),
            m_scale * scale,
            m_skew
        );

        Transform::UpdateMatrix();
    }

    void Transform2D::SetGlobalTranslation(const SR_MATH_NS::FVector3& translation) {
        if (auto&& pParent = GetParentTransform()) {
            auto matrix = SR_MATH_NS::Matrix4x4::FromTranslate(translation);
            matrix *= SR_MATH_NS::Matrix4x4::FromScale(m_skew);
            matrix *= SR_MATH_NS::Matrix4x4::FromEulers(m_rotation.Inverse());
            matrix *= SR_MATH_NS::Matrix4x4::FromScale(m_scale);

            matrix = pParent->GetMatrix().Inverse() * matrix;

            m_translation = matrix.GetTranslate();

            UpdateTree();
        }
        else {
            SetTranslation(translation);
        }
    }

    void Transform2D::SetGlobalRotation(const SR_MATH_NS::FVector3& euler) {
        if (auto&& pParent = GetParentTransform()) {
            auto&& matrix = SR_MATH_NS::Matrix4x4::FromScale(SR_MATH_NS::FVector3(1) / m_skew);
            matrix *= SR_MATH_NS::Matrix4x4::FromEulers(euler);
            matrix *= SR_MATH_NS::Matrix4x4::FromScale(m_scale);

            matrix = pParent->GetMatrix().Inverse() * matrix;

            SetRotation(matrix.GetEulers());
        }
        else {
            auto&& matrix = SR_MATH_NS::Matrix4x4::FromScale(SR_MATH_NS::FVector3(1) / m_skew);
            matrix *= SR_MATH_NS::Matrix4x4::FromEulers(euler);
            matrix *= SR_MATH_NS::Matrix4x4::FromScale(m_scale);

            SetRotation(matrix.GetEulers());
        }
    }

    void Transform2D::SetAnchor(Anchor anchorType) {
        m_anchor = anchorType;
        UpdateTree();
    }

    void Transform2D::SetStretch(Stretch stretch) {
        m_stretch = stretch;
        UpdateTree();
    }

    Transform *Transform2D::Copy() const {
        auto&& pTransform = new Transform2D();

        pTransform->m_anchor = m_anchor;
        pTransform->m_stretch = m_stretch;

        pTransform->m_translation = m_translation;
        pTransform->m_rotation = m_rotation;
        pTransform->m_scale = m_scale;
        pTransform->m_skew = m_skew;

        return pTransform;
    }

    SR_MATH_NS::FVector3 Transform2D::CalculateStretch() const {
        auto&& pParent = dynamic_cast<Transform2D*>(GetParentTransform());
        if (!pParent) {
            return SR_MATH_NS::FVector3::One();
        }

        auto&& aspect = pParent->GetScale().XY().Aspect();
        if (SR_EQUALS(aspect, 0.f)) {
            return SR_MATH_NS::FVector3();
        }

        auto scale = SR_MATH_NS::FVector3::One();

        auto&& fitWidth = [&]() {
            scale.x *= 1.f / aspect;

            //if (translation.x > 0) {
            //    translation.x += (aspect - 1.f) * scale.x;
            //}
            //else if (translation.x < 0) {
            //    translation.x -= (aspect - 1.f) * scale.x;
            //}
        };

        auto&& fitHeight = [&]() {
            scale.y *= aspect;

           //if (translation.y > 0) {
           //    translation.y += (1.f - aspect) * scale.y;
           //}
           //else if (translation.y < 0) {
           //    translation.y -= (1.f - aspect) * scale.y;
           //}
        };

        //auto&& otherAspect = (float)screenSize.Width() / (float)screenSize.Height();
        //auto&& myAspect = (float)_refferenceW / (float)_refferenceH;

        /// Компенсация растяжения родительской ноды
        switch (m_stretch) {
            case Stretch::ChangeAspect:
                break;
            case Stretch::WidthControlsHeight:
                fitWidth();
                break;
            case Stretch::HeightControlsWidth:
                fitHeight();
                break;
            case Stretch::ShowAll: {
                if (aspect > 1.f) {
                    fitWidth();
                }
                else {
                    fitHeight();
                }
                break;
            }
            case Stretch::NoBorder: {
                if (aspect > 1.f) {
                    fitHeight();
                }
                else {
                    fitWidth();
                }
                break;
            }
            default:
                break;
        }

        return scale;
    }

    SR_MATH_NS::FVector3 Transform2D::CalculateAnchor(const SR_MATH_NS::FVector3& scale) const {
        auto&& pParent = dynamic_cast<Transform2D*>(GetParentTransform());
        if (!pParent) {
            return SR_MATH_NS::FVector3();
        }

        auto&& parentScale = pParent->GetScale();

        auto&& horizontalAspect = (parentScale / m_scale).XY().Aspect();
        auto&& horizontalAnchor = (horizontalAspect - 1.f) * (1.f / horizontalAspect);

        auto&& verticalAspect = (parentScale / m_scale).XY().AspectInv();
        auto&& verticalAnchor = (verticalAspect - 1.f) * (1.f / verticalAspect);

        switch (m_anchor) {
            case Anchor::None:
            case Anchor::MiddleCenter:
                return m_translation;

            case Anchor::MiddleLeft:
                return m_translation + SR_MATH_NS::FVector3(-horizontalAnchor, 0.f, 0.f);
            case Anchor::MiddleRight:
                return m_translation + SR_MATH_NS::FVector3(horizontalAnchor, 0.f, 0.f);

            case Anchor::TopCenter:
                return m_translation + SR_MATH_NS::FVector3(0.f, verticalAnchor, 0.f);
            case Anchor::BottomCenter:
                return m_translation + SR_MATH_NS::FVector3(0.f, -verticalAnchor, 0.f);

            case Anchor::TopLeft:
                return m_translation + SR_MATH_NS::FVector3(-horizontalAnchor, verticalAnchor, 0.f);
            case Anchor::TopRight:
                return m_translation + SR_MATH_NS::FVector3(horizontalAnchor, verticalAnchor, 0.f);

            case Anchor::BottomLeft:
                return m_translation + SR_MATH_NS::FVector3(-horizontalAnchor, -verticalAnchor, 0.f);
            case Anchor::BottomRight:
                return m_translation + SR_MATH_NS::FVector3(horizontalAnchor, -verticalAnchor, 0.f);

            default:
                return SR_MATH_NS::FVector3();
        }
    }

    int32_t Transform2D::GetPriority() { /// NOLINT
        if (!m_isDirtyPriority) {
            return m_priority;
        }

        if (m_relativePriority) {
            if (auto&& pParentTransform = dynamic_cast<Transform2D*>(GetParentTransform())) {
                m_priority = m_localPriority + pParentTransform->GetPriority();
            }
            else {
                m_priority = m_localPriority;
            }
        }
        else {
            m_priority = m_localPriority;
        }

        m_isDirtyPriority = false;

        return m_priority;
    }
}