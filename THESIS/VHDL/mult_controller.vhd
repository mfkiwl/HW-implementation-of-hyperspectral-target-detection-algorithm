----------------------------------------------------------------------------------
-- Company: 
-- Engineer: Dordije Boskovic
-- 
-- Create Date: 
-- Design Name: 
-- Module Name: Dot product controller - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------

library IEEE;
use IEEE . STD_LOGIC_1164 . all;
use ieee . numeric_std . all;

entity mult_controller is
	generic (
		PIPELINE_DEPTH : integer := 2
	);
	port (
		clk     : in std_logic;
		en      : in std_logic;
		reset_n : in std_logic;
		p_rdy   : out std_logic
	);
end mult_controller;

architecture Behavioral of mult_controller is

	signal counter : integer range 0 to PIPELINE_DEPTH + 3;
	signal out_rdy : std_logic;

begin

	out_rdy <= '1' when (counter = PIPELINE_DEPTH and en = '1') else '0';
	p_rdy   <= out_rdy;

	process (clk, reset_n)
	begin
		if (rising_edge (clk)) then
			if (reset_n = '0') then
				counter <= 0;
			elsif (en = '1') then

				if (counter = PIPELINE_DEPTH) then
					counter <= 1;
				else
					counter <= counter + 1;
				end if;
				
			end if;
		end if;

	end process;
	
end Behavioral;