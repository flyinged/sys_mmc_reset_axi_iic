--Copyright 1986-2016 Xilinx, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2016.2 (win64) Build 1577090 Thu Jun  2 16:32:40 MDT 2016
--Date        : Thu Dec 22 16:48:02 2016
--Host        : PC10514 running 64-bit Service Pack 1  (build 7601)
--Command     : generate_target design_1_wrapper.bd
--Design      : design_1_wrapper
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_1_wrapper is
  port (
    clk_sys_clk3_125_n : in STD_LOGIC;
    clk_sys_clk3_125_p : in STD_LOGIC;
    p0_ctrl_en : out STD_LOGIC_VECTOR ( 0 to 0 );
    p0_ctrl_iic_5_scl_io : inout STD_LOGIC;
    p0_ctrl_iic_5_sda_io : inout STD_LOGIC;
    p0_ctrl_slave_pwr_en_out_b : out STD_LOGIC_VECTOR ( 0 to 0 );
    pwr_ctrl_hold_pwr : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_hse_l_0 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_l_1 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_m_0 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_m_1 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_r_0 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_r_1 : out STD_LOGIC_VECTOR ( 0 to 0 )
  );
end design_1_wrapper;

architecture STRUCTURE of design_1_wrapper is
  component design_1 is
  port (
    p0_ctrl_iic_5_scl_i : in STD_LOGIC;
    p0_ctrl_iic_5_scl_o : out STD_LOGIC;
    p0_ctrl_iic_5_scl_t : out STD_LOGIC;
    p0_ctrl_iic_5_sda_i : in STD_LOGIC;
    p0_ctrl_iic_5_sda_o : out STD_LOGIC;
    p0_ctrl_iic_5_sda_t : out STD_LOGIC;
    clk_sys_clk3_125_n : in STD_LOGIC;
    clk_sys_clk3_125_p : in STD_LOGIC;
    sys_led_r_0 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_r_1 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_m_0 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_m_1 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_hse_l_0 : out STD_LOGIC_VECTOR ( 0 to 0 );
    sys_led_l_1 : out STD_LOGIC_VECTOR ( 0 to 0 );
    pwr_ctrl_hold_pwr : out STD_LOGIC_VECTOR ( 0 to 0 );
    p0_ctrl_slave_pwr_en_out_b : out STD_LOGIC_VECTOR ( 0 to 0 );
    p0_ctrl_en : out STD_LOGIC_VECTOR ( 0 to 0 )
  );
  end component design_1;
  component IOBUF is
  port (
    I : in STD_LOGIC;
    O : out STD_LOGIC;
    T : in STD_LOGIC;
    IO : inout STD_LOGIC
  );
  end component IOBUF;
  signal p0_ctrl_iic_5_scl_i : STD_LOGIC;
  signal p0_ctrl_iic_5_scl_o : STD_LOGIC;
  signal p0_ctrl_iic_5_scl_t : STD_LOGIC;
  signal p0_ctrl_iic_5_sda_i : STD_LOGIC;
  signal p0_ctrl_iic_5_sda_o : STD_LOGIC;
  signal p0_ctrl_iic_5_sda_t : STD_LOGIC;
begin
design_1_i: component design_1
     port map (
      clk_sys_clk3_125_n => clk_sys_clk3_125_n,
      clk_sys_clk3_125_p => clk_sys_clk3_125_p,
      p0_ctrl_en(0) => p0_ctrl_en(0),
      p0_ctrl_iic_5_scl_i => p0_ctrl_iic_5_scl_i,
      p0_ctrl_iic_5_scl_o => p0_ctrl_iic_5_scl_o,
      p0_ctrl_iic_5_scl_t => p0_ctrl_iic_5_scl_t,
      p0_ctrl_iic_5_sda_i => p0_ctrl_iic_5_sda_i,
      p0_ctrl_iic_5_sda_o => p0_ctrl_iic_5_sda_o,
      p0_ctrl_iic_5_sda_t => p0_ctrl_iic_5_sda_t,
      p0_ctrl_slave_pwr_en_out_b(0) => p0_ctrl_slave_pwr_en_out_b(0),
      pwr_ctrl_hold_pwr(0) => pwr_ctrl_hold_pwr(0),
      sys_led_hse_l_0(0) => sys_led_hse_l_0(0),
      sys_led_l_1(0) => sys_led_l_1(0),
      sys_led_m_0(0) => sys_led_m_0(0),
      sys_led_m_1(0) => sys_led_m_1(0),
      sys_led_r_0(0) => sys_led_r_0(0),
      sys_led_r_1(0) => sys_led_r_1(0)
    );
p0_ctrl_iic_5_scl_iobuf: component IOBUF
     port map (
      I => p0_ctrl_iic_5_scl_o,
      IO => p0_ctrl_iic_5_scl_io,
      O => p0_ctrl_iic_5_scl_i,
      T => p0_ctrl_iic_5_scl_t
    );
p0_ctrl_iic_5_sda_iobuf: component IOBUF
     port map (
      I => p0_ctrl_iic_5_sda_o,
      IO => p0_ctrl_iic_5_sda_io,
      O => p0_ctrl_iic_5_sda_i,
      T => p0_ctrl_iic_5_sda_t
    );
end STRUCTURE;
