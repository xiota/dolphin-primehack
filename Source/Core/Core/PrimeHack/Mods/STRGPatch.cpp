#include "Core/PrimeHack/Mods/STRGPatch.h"

#include "Core/PowerPC/PowerPC.h"
#include "Core/PowerPC/MMU.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/System.h"

namespace prime {
namespace {
std::string readin_str(PowerPC::MMU& mmu, u32 str_ptr) {
  std::ostringstream key_readin;

  for (char c = mmu.Read_U8(str_ptr); c; c = mmu.Read_U8(++str_ptr)) {
    key_readin << c;
  }
  return key_readin.str();
}

u32 bsearch_strg_table(PowerPC::MMU& mmu, std::string const& key, u32 strg_header) {
  u32 bsearch_left = mmu.Read_U32(strg_header + 0x14);
  int dist = mmu.Read_U32(strg_header + 0x8);
  while (dist > 0) {
    int midpoint_offset = (dist * 4) & ~0x7;
    int half_dist = dist >> 1;
    std::string test_key = readin_str(mmu, mmu.Read_U32(bsearch_left + midpoint_offset));
    if (test_key.compare(key) < 0) {
      dist -= (1 + half_dist);
      bsearch_left += midpoint_offset + 8;
    } else {
      dist = half_dist;
    }
  }
  return bsearch_left;
}

void patch_strg_entry_mp3(PowerPC::PowerPCState& ppc_state, PowerPC::MMU& mmu, u32 vers) {
  u32 strg_header = ppc_state.gpr[31], key_ptr = ppc_state.gpr[4];

  u32 patched_table_addr = 0;
  switch (vers) {
    case 0:
      patched_table_addr = 0x80676c00;
    case 1:
      patched_table_addr = 0x8067a400;
    case 2:
      patched_table_addr = 0x80684800;
    default:
      break;
  }
  std::string key = readin_str(mmu, key_ptr);
  if (key == "ShakeOffGandrayda") {
    u32 bsearch_result = bsearch_strg_table(mmu, key, strg_header);
    std::string found_key = readin_str(mmu, mmu.Read_U32(bsearch_result));
    if (found_key == key) {
      u32 strg_val_index = mmu.Read_U32(bsearch_result + 4);
      u32 strg_val_table = mmu.Read_U32(strg_header + 0x1c);
      mmu.Write_U32(patched_table_addr, strg_val_table + 4 * strg_val_index);
    }
  }
  ppc_state.gpr[3] = ppc_state.gpr[31];
}
}

void STRGPatch::run_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_1:
  case Game::PRIME_1_GCN:
  case Game::PRIME_1_GCN_R1:
  case Game::PRIME_1_GCN_R2:
  case Game::PRIME_2:
  case Game::PRIME_2_GCN:
    break;
  case Game::PRIME_3_STANDALONE:
  case Game::PRIME_3:
    run_mod_mp3();
    break;
  default:
    break;
  }
}

void STRGPatch::run_mod_mp3() {
  char str[] = "&just=center;Mash Jump [&image=0x5FC17B1F30BAA7AE;] to shake off Gandrayda!";
  for (size_t i = 0; i < sizeof(str); i++) {
    write8(str[i], replace_string_addr + static_cast<u32>(i));
  }
}

bool STRGPatch::init_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_1:
  case Game::PRIME_1_GCN:
  case Game::PRIME_1_GCN_R1:
  case Game::PRIME_1_GCN_R2:
  case Game::PRIME_2:
  case Game::PRIME_2_GCN:
    break;
  case Game::PRIME_3_STANDALONE: {
    int vmc_id = Core::System::GetInstance().GetPowerPC().RegisterVmcall(patch_strg_entry_mp3);
    if (region == Region::NTSC_U) {
      replace_string_addr = 0x80684800;
      add_code_change(0x803cdd64, gen_vmcall(vmc_id, 2));
    }
    break;
  }
  case Game::PRIME_3: {
    int vmc_id = Core::System::GetInstance().GetPowerPC().RegisterVmcall(patch_strg_entry_mp3);
    if (region == Region::NTSC_U) {
      replace_string_addr = 0x80676c00;
      add_code_change(0x803cc3f4, gen_vmcall(vmc_id, 0));
    } else if (region == Region::PAL) {
      replace_string_addr = 0x8067a400;
      add_code_change(0x803cbb10, gen_vmcall(vmc_id, 1));
    }
    break;
  }
  default:
    break;
  }
  return true;
}

}
