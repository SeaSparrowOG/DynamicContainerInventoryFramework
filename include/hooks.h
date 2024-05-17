#pragma once

namespace Hooks
{
    void Install() noexcept;

    /*
    Credit to ThirdEyeSqueegee for this.
    Nexus: https://next.nexusmods.com/profile/ThirdEye3301/about-me
    Github: https://github.com/ThirdEyeSqueegee

    Note: This runs once per reference, but once per load. Hence the need to serde.
    */
    class InitItemImpl : public clib_util::singleton::ISingleton<InitItemImpl>{
    public:
        static void Thunk(RE::TESObjectREFR* a_ref) noexcept;
        inline static REL::Relocation<decltype(&Thunk)> func;
        static constexpr std::size_t idx{ 19 }; // 0x13
    };
}