----------------------------------------------------------------------------------
-- Company: Taksun
-- Engineer: Hosseinali
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity img_real_to_cmplx is
    generic(
        img_real_tDATA_Width : integer := 16
    );
    Port ( 
        
            clk                 : in STD_LOGIC; 
            S_AXIS_IMG_tDATA    : in std_logic_vector(img_real_tDATA_Width-1 downto 0); 
            S_AXIS_IMG_tVALID   : in STD_LOGIC; 
            S_AXIS_REAL_tDATA   : in std_logic_vector(img_real_tDATA_Width-1 downto 0); 
            S_AXIS_REAL_tVALID  : in STD_LOGIC; 
            M_AXIS_CMPLX_tDATA  : out std_logic_vector(img_real_tDATA_Width*2-1 downto 0); 
            M_AXIS_CMPLX_tVALID : out std_logic
            
        );
end img_real_to_cmplx;

architecture Behavioral of img_real_to_cmplx is

begin
M_AXIS_CMPLX_tDATA(img_real_tDATA_Width-1 downto 0)                         <= S_AXIS_REAL_tDATA when S_AXIS_REAL_tVALID = '1' else (others=>'0');
M_AXIS_CMPLX_tDATA(img_real_tDATA_Width*2-1 downto img_real_tDATA_Width)    <= S_AXIS_IMG_tDATA when S_AXIS_IMG_tVALID = '1' else (others=>'0');
M_AXIS_CMPLX_tVALID                                                         <= S_AXIS_REAL_tVALID or S_AXIS_IMG_tVALID;
end Behavioral;
