#include <type_traits>
#include <unordered_map>

namespace state_machine {

template<typename T>
concept EnumType = std::is_enum_v<T>;

template <typename... Enums>
auto enum_to_tuple(Enums... enums) {
    return std::make_tuple(enums...);
}

template<EnumType StateEnum, typename Context>
class StateMachine {
public:
    // Expose template arguments.
    using StateEnumType = StateEnum;
    using ContextType = Context;

    class StateAction {
    public:
        virtual ~StateAction() = default;
        virtual void EnterState(Context* context) {}
        virtual void ExitState(Context* context) {}
        virtual StateEnum NextState(Context* context) = 0;
    };

    StateMachine(Context* context) : context_(context) {}

    void AddState(StateEnum state_enum, std::unique_ptr<StateAction>&& action) {
        state_actions_[state_enum] = std::move(action);
    }

    void Start(StateEnum start_enum) { 
        enum_ = start_enum; 
        state_actions_[enum_]->EnterState(context_);
    }

    void Update() {
        StateEnum next_enum = state_actions_[enum_]->NextState(context_);
        if (next_enum != enum_) {
            state_actions_[enum_]->ExitState(context_);
            state_actions_[next_enum]->EnterState(context_);
            enum_ = next_enum;
        }
    }

    Context* MutableContext() { return context_; }
private:
    StateEnum enum_;
    Context* context_;
    std::unordered_map<StateEnum, std::unique_ptr<StateAction>> state_actions_;
};


}  // namespace state_machine
