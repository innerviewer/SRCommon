//
// Created by Monika on 12.12.2022.
//

#include <Utils/ECS/IComponentable.h>
#include <Utils/ECS/Component.h>
#include <Utils/ECS/GameObject.h>

namespace SR_UTILS_NS {
    bool IComponentable::IsDirty() const noexcept {
        return m_dirty;
    }

    SR_HTYPES_NS::Marshal::Ptr IComponentable::SaveComponents(SR_HTYPES_NS::Marshal::Ptr pMarshal, SavableFlags flags) const {
        pMarshal->Write(static_cast<uint32_t>(m_components.size()));
        for (auto&& pComponent : m_components) {
            auto&& marshalComponent = pComponent->Save(nullptr, flags);
            pMarshal->Write<uint64_t>(marshalComponent->BytesCount());
            pMarshal->Append(marshalComponent);
        }

        return pMarshal;
    }

    Component* IComponentable::GetComponent(const std::string &name) {
        for (auto&& pComponent : m_components) {
            /// TODO: переделать на хеши
            if (pComponent->GetComponentName() == name) {
                return pComponent;
            }
        }

        return nullptr;
    }

    bool IComponentable::ContainsComponent(const std::string &name) {
        return GetComponent(name);
    }

    Component* IComponentable::GetOrCreateComponent(const std::string &name) {
        if (auto&& pComponent = GetComponent(name)) {
            return pComponent;
        }

        if (auto&& pComponent = ComponentManager::Instance().CreateComponentOfName(name)) {
            if (AddComponent(pComponent)) {
                return pComponent;
            }
            else {
                SRHalt("IComponentable::GetOrCreateComponent() : failed to add component!");
            }
        }

        return nullptr;
    }

    Component* IComponentable::GetComponent(size_t id) {
        for (auto&& pComponent : m_components) {
            if (pComponent->GetComponentId() != id) {
                continue;
            }

            return pComponent;
        }

        return nullptr;
    }

    void IComponentable::ForEachComponent(const std::function<bool(Component *)> &fun) {
        for (auto&& component : m_components) {
            if (!fun(component)) {
                break;
            }
        }
    }

    bool IComponentable::LoadComponent(Component *pComponent) {
        if (!pComponent) {
            SRHalt("pComponent is nullptr!");
            return false;
        }

        m_loadedComponents.emplace_back(pComponent);

        /// pComponent->OnAttached();
        /// Здесь нельзя аттачить, иначе будет очень трудно отлавливаемый deadlock

        SetDirty(true);

        return true;
    }

    bool IComponentable::AddComponent(Component* pComponent) {
        m_components.emplace_back(pComponent);
        ++m_componentsCount;

        if (auto&& pGameObject = dynamic_cast<GameObject*>(this)) {
            pComponent->SetParent(pGameObject);
        }
        else {
            SRHalt("Unknown parent!");
            return false;
        }

        pComponent->OnAttached();
        pComponent->OnMatrixDirty();

        SetDirty(true);

        return true;
    }

    bool IComponentable::RemoveComponent(Component *component) {
        for (auto it = m_components.begin(); it != m_components.end(); ++it) {
            if (*it != component) {
                continue;
            }

            if (component->GetParent() != this) {
                SRHalt("Game object not are children!");
            }

            component->OnDestroy();

            m_components.erase(it);
            --m_componentsCount;

            SetDirty(true);

            return true;
        }

        SR_ERROR("IComponentable::RemoveComponent() : component \"" + component->GetComponentName() + "\" not found!");

        return false;
    }

    bool IComponentable::ReplaceComponent(Component *source, Component *destination) {
        for (auto it = m_components.begin(); it != m_components.end(); ++it) {
            if (*it == source) {
                source->OnDestroy();
                *it = destination;

                if (auto&& pGameObject = dynamic_cast<GameObject*>(this)) {
                    destination->SetParent(pGameObject);
                }
                else {
                    SRHalt("Unknown parent!");
                    return false;
                }

                destination->OnAttached();
                destination->OnMatrixDirty();

                SetDirty(true);

                return true;
            }
        }

        SR_ERROR("IComponentable::ReplaceComponent() : component \"" + source->GetComponentName() + "\" not found!");

        return false;
    }

    bool IComponentable::PostLoad() {
        if (!m_dirty) {
            return false;
        }

        if (!m_loadedComponents.empty()) {
            m_components.reserve(m_loadedComponents.size());

            for (auto&& pLoadedCmp : m_loadedComponents) {
                AddComponent(pLoadedCmp);
                pLoadedCmp->OnMatrixDirty();
            }

            m_loadedComponents.clear();
        }

        return true;
    }

    void IComponentable::Awake(bool isPaused) noexcept {
        if (!m_dirty) {
            return;
        }

        for (auto&& pComponent : m_components) {
            if (isPaused && !pComponent->ExecuteInEditMode()) {
                continue;
            }

            if (pComponent->IsAwake()) {
                continue;
            }

            pComponent->Awake();
        }
    }

    void IComponentable::Start() noexcept {
        if (!m_dirty) {
            return;
        }

        for (auto&& pComponent : m_components) {
            if (!pComponent->IsAwake()) {
                continue;
            }

            if (pComponent->IsStarted()) {
                continue;
            }

            pComponent->Start();
        }
    }

    void IComponentable::CheckActivity() noexcept {
        for (auto&& pComponent : m_components) {
            if (!pComponent->IsAwake()) {
                continue;
            }

            pComponent->CheckActivity();
        }
    }

    void IComponentable::DestroyComponents() {
        for (auto&& pComponent : m_components) {
            pComponent->OnDestroy();
        }

        m_components.clear();
        m_componentsCount = 0;
    }
}