/*******************************************************************************
  Policy Manager Source file

  Company:
    Microchip Technology Inc.

  File Name:
    policy_manager.c

  Description:
    This file contains the function definitions for Policy Manager functions
 *******************************************************************************/
/*******************************************************************************
Copyright �  [2019-2020] Microchip Technology Inc. and its subsidiaries.

Subject to your compliance with these terms, you may use Microchip software and
any derivatives exclusively with Microchip products. It is your responsibility
to comply with third party license terms applicable to your use of third party
software (including open source software) that may accompany Microchip software.

THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER EXPRESS,
IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES
OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN
NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN
ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST
EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU
HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#include <psf_stdinc.h>

void DPM_Init(UINT8 u8PortNum)
{
    UINT8 u8DPM_Status = gasDPM[u8PortNum].u8DPM_Status;
    UINT8 u8DPM_ConfigData = gasDPM[u8PortNum].u8DPM_ConfigData;
    
    u8DPM_Status |= (CONFIG_PD_DEFAULT_SPEC_REV << DPM_CURR_PD_SPEC_REV_POS);
    u8DPM_ConfigData |= (CONFIG_PD_DEFAULT_SPEC_REV  << DPM_DEFAULT_PD_SPEC_REV_POS);
        
    if((gasCfgStatusData.sPerPortData[u8PortNum].u32CfgData & TYPEC_PORT_TYPE_MASK)== (PD_ROLE_SOURCE))
    {   
        /* Set Port Power Role as Source in DPM Configure variable*/
        u8DPM_ConfigData |= (PD_ROLE_SOURCE << DPM_DEFAULT_POWER_ROLE_POS); 
        
        /* Set Port Data Role as DFP in DPM Configure variable*/
        u8DPM_ConfigData |= (PD_ROLE_DFP << DPM_DEFAULT_DATA_ROLE_POS);
        
        /* Set Port Power Role as Source in DPM Status variable */
        u8DPM_Status |= (PD_ROLE_SOURCE << DPM_CURR_POWER_ROLE_POS);
        
        /* Set Port Data Role as DFP in DPM Status variable */
        u8DPM_Status |= (PD_ROLE_DFP << DPM_CURR_DATA_ROLE_POS);
        
        /* Set Port Power Role as Source in Port Connection Status register */
        gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus |= DPM_PORT_POWER_ROLE_STATUS; 
        
        /* Set Port Data Role as DFP in Port Connection Status register */
        gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus |= DPM_PORT_DATA_ROLE_STATUS; 
    }       
    else
    {
        /* Set the Default Port Power Role as Sink in DPM Status variable */
        u8DPM_ConfigData |= (PD_ROLE_SINK << DPM_DEFAULT_POWER_ROLE_POS);
        
        /* Set the Default Port Data Role as UFP in DPM Status variable */
        u8DPM_ConfigData |= (PD_ROLE_UFP << DPM_DEFAULT_DATA_ROLE_POS);
        
        /* Set the Current Port Power Role as Sink in DPM Status variable */
        u8DPM_Status |= (PD_ROLE_SINK << DPM_CURR_POWER_ROLE_POS);
        
        /* Set the Current  Port Data Role as UFP in DPM Status variable */
        u8DPM_Status |= (PD_ROLE_UFP << DPM_CURR_DATA_ROLE_POS);
        
        /* Set Port Power Role as Sink in Port Connection Status register */
        gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus &= ~(DPM_PORT_POWER_ROLE_STATUS); 
        
        /* Set Port Data Role as UFP in Port Connection Status register */
        gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus &= ~(DPM_PORT_DATA_ROLE_STATUS);         
        /*On initialization Adversited PDO is updated to Sink's PDO*/
        (void)MCHP_PSF_HOOK_MEMCPY(gasCfgStatusData.sPerPortData[u8PortNum].u32aAdvertisedPDO, 
            gasCfgStatusData.sPerPortData[u8PortNum].u32aSinkPDO, 
            (gasCfgStatusData.sPerPortData[u8PortNum].u8SinkPDOCnt * 4));
        /*Advertised PDO Count is updated to SinkPDO Count*/
        gasCfgStatusData.sPerPortData[u8PortNum].u8AdvertisedPDOCnt = \
                        gasCfgStatusData.sPerPortData[u8PortNum].u8SinkPDOCnt;
    }
    
    gasDPM[u8PortNum].u8DPM_Status =  u8DPM_Status;
    gasDPM[u8PortNum].u8DPM_ConfigData  = u8DPM_ConfigData;
	
#if (TRUE == INCLUDE_POWER_FAULT_HANDLING)
	gasDPM[u8PortNum].u8VBUSPowerGoodTmrID = MAX_CONCURRENT_TIMERS;
	gasDPM[u8PortNum].u8PowerFaultISR = SET_TO_ZERO;
	gasDPM[u8PortNum].u8VBUSPowerFaultCount = RESET_TO_ZERO;
	gasDPM[u8PortNum].u8HRCompleteWait = RESET_TO_ZERO;
    /*VCONN OCS related variables*/
    gasDPM[u8PortNum].u8VCONNGoodtoSupply = TRUE;
    gasDPM[u8PortNum].u8VCONNPowerGoodTmrID = MAX_CONCURRENT_TIMERS;
    gasDPM[u8PortNum].u8VCONNPowerFaultCount = SET_TO_ZERO;

#endif
	
    
}
/********************************************************************************************/

void DPM_StateMachineInit(void)
{
	for (UINT8 u8PortNum = 0; u8PortNum < CONFIG_PD_PORT_COUNT; u8PortNum++)
  	{
        
        if (UPD_PORT_ENABLED == ((gasCfgStatusData.sPerPortData[u8PortNum].u32CfgData \
                                    & TYPEC_PORT_ENDIS_MASK) >> TYPEC_PORT_ENDIS_POS))
        {
		  	/* Init UPD350 GPIO */
		  	UPD_GPIOInit(u8PortNum);
			
            /*Type-C UPD350 register configuration for a port*/
            TypeC_InitPort(u8PortNum);
            
            /* Protocol Layer initialization for all the port present */
            PRL_Init (u8PortNum);
        }
    }
}
/*******************************************************************************/

void DPM_RunStateMachine (UINT8 u8PortNum)
{
    MCHP_PSF_HOOK_DPM_PRE_PROCESS(u8PortNum);
    
    /* Run Type C State machine*/
    TypeC_RunStateMachine (u8PortNum);
    
    /* Run Policy engine State machine*/
    PE_RunStateMachine(u8PortNum);
	
	/* Power Fault handling*/
	#if (TRUE == INCLUDE_POWER_FAULT_HANDLING)
		DPM_PowerFaultHandler(u8PortNum);
	#endif
    
    /* UPD Power Management */
    #if (TRUE == INCLUDE_POWER_MANAGEMENT_CTRL)
        UPD_PwrManagementCtrl (u8PortNum);
    #endif
}

/****************************** DPM APIs Accessing Type C Port Control Module*********************/
void DPM_GetTypeCStates(UINT8 u8PortNum, UINT8 *pu8TypeCState, UINT8 *pu8TypeCSubState)
{
    *pu8TypeCState = gasTypeCcontrol[u8PortNum].u8TypeCState;
    *pu8TypeCSubState = gasTypeCcontrol[u8PortNum].u8TypeCSubState;
}
void DPM_SetTypeCState(UINT8 u8PortNum, UINT8 u8TypeCState, UINT8 u8TypeCSubState)
{
    gasTypeCcontrol[u8PortNum].u8TypeCState = u8TypeCState;
    gasTypeCcontrol[u8PortNum].u8TypeCSubState = u8TypeCSubState;
}
void DPM_GetPoweredCablePresence(UINT8 u8PortNum, UINT8 *pu8RaPresence)
{
    *pu8RaPresence = (gasTypeCcontrol[u8PortNum].u8PortSts & TYPEC_PWDCABLE_PRES_MASK);
}
/*******************************************************************************/

/**************************DPM APIs for VCONN *********************************/
void DPM_VConnOnOff(UINT8 u8PortNum, UINT8 u8VConnEnable)
{
    if(u8VConnEnable == DPM_VCONN_ON)
    {
        /*Enable VCONN by switching on the VCONN FETS*/
        TypeC_EnabDisVCONN (u8PortNum, TYPEC_VCONN_ENABLE);              
    }    
    else
    {
        /*Disable VCONN by switching off the VCONN FETS*/
        TypeC_EnabDisVCONN (u8PortNum, TYPEC_VCONN_DISABLE);     
    }
}
/*******************************************************************************/

/**************************DPM APIs for VBUS* *********************************/
void DPM_SetPortPower(UINT8 u8PortNum)
{
    UINT16 u16PDOVoltage = DPM_GET_VOLTAGE_FROM_PDO_MILLI_V(gasDPM[u8PortNum].u32NegotiatedPDO);
    UINT16 u16PDOCurrent = DPM_GET_CURRENT_FROM_PDO_MILLI_A(gasDPM[u8PortNum].u32NegotiatedPDO);
	
    TypeC_ConfigureVBUSThr(u8PortNum, u16PDOVoltage, u16PDOCurrent, TYPEC_CONFIG_NON_PWR_FAULT_THR);
	
    if (DPM_GET_CURRENT_POWER_ROLE(u8PortNum) == PD_ROLE_SOURCE)
    {
		PWRCTRL_SetPortPower (u8PortNum, gasDPM[u8PortNum].u8NegotiatedPDOIndex, 
                u16PDOVoltage, u16PDOCurrent);
    }	
}

void DPM_TypeCVBus5VOnOff(UINT8 u8PortNum, UINT8 u8VbusOnorOff)
{
    UINT16 u16Current = gasDPM[u8PortNum].u16MaxCurrSupportedin10mA * 10;
  	if (DPM_VBUS_ON == u8VbusOnorOff)
    {
        TypeC_ConfigureVBUSThr(u8PortNum, TYPEC_VBUS_5V, u16Current, TYPEC_CONFIG_NON_PWR_FAULT_THR);
 		PWRCTRL_SetPortPower (u8PortNum, DPM_VSAFE5V_PDO_INDEX_1, TYPEC_VBUS_5V, u16Current);
    }
    else
    {
        TypeC_ConfigureVBUSThr(u8PortNum, TYPEC_VBUS_0V, u16Current, TYPEC_CONFIG_NON_PWR_FAULT_THR);
		PWRCTRL_SetPortPower (u8PortNum, DPM_VSAFE0V_PDO_INDEX, TYPEC_VBUS_0V, u16Current);
		PWRCTRL_ConfigVBUSDischarge (u8PortNum, TRUE);
    }
}

UINT16 DPM_GetVBUSVoltage(UINT8 u8PortNum)
{
  UINT8 u8VBUSPresence = ((gasTypeCcontrol[u8PortNum].u8IntStsISR & 
                          TYPEC_VBUS_PRESENCE_MASK) >> TYPEC_VBUS_PRESENCE_POS);
  if (u8VBUSPresence!= TYPEC_VBUS_0V_PRES)
  {
      return DPM_GET_VOLTAGE_FROM_PDO_MILLI_V(gasCfgStatusData.sPerPortData[u8PortNum].u32aAdvertisedPDO[--u8VBUSPresence]);
  }
  else
  {
      return TYPEC_VBUS_0V_PRES;
  }
}

void DPM_EnablePowerFaultDetection(UINT8 u8PortNum)
{	
	#if (TRUE == INCLUDE_POWER_FAULT_HANDLING)
	/* Obtain voltage from negoitated PDO*/
    UINT16 u16PDOVoltage = DPM_GET_VOLTAGE_FROM_PDO_MILLI_V(gasDPM[u8PortNum].u32NegotiatedPDO);
    UINT16 u16PDOCurrent = DPM_GET_CURRENT_FROM_PDO_MILLI_A(gasDPM[u8PortNum].u32NegotiatedPDO);
	/* set the threshold to detect fault*/
	TypeC_ConfigureVBUSThr(u8PortNum, u16PDOVoltage, u16PDOCurrent, TYPEC_CONFIG_PWR_FAULT_THR);
	#endif
}
/*******************************************************************************/
#if (TRUE == INCLUDE_PD_SOURCE)

/****************************** DPM Source related APIs*****************************************/
/* Validate the received Request message */
UINT8 DPM_ValidateRequest(UINT8 u8PortNum, UINT16 u16Header, UINT8 *u8DataBuf)
{
    UINT8 u8RetVal = FALSE;
    UINT8 u8SinkReqObjPos= SET_TO_ZERO;
    UINT16 u16SinkReqCurrVal = SET_TO_ZERO;
    UINT16 u16SrcPDOCurrVal = SET_TO_ZERO;
    UINT8 u8RaPresence = SET_TO_ZERO;
    
    /* Get the status of E-Cable presence and ACK status */
    u8RaPresence = (gasTypeCcontrol[u8PortNum].u8PortSts & TYPEC_PWDCABLE_PRES_MASK) & \
                    (~((gasPolicy_Engine[u8PortNum].u8PEPortSts & \
                                            PE_CABLE_RESPOND_NAK) >> PE_CABLE_RESPOND_NAK_POS));
    
    /* Get the Requested PDO object position from received buffer */
    u8SinkReqObjPos= ((u8DataBuf[INDEX_3]) & PE_REQUEST_OBJ_MASK) >> PE_REQUEST_OBJ_POS;
    
    /* Get the Requested current value */
    u16SinkReqCurrVal = (UINT16)(((MAKE_UINT32_FROM_BYTES(u8DataBuf[INDEX_0], u8DataBuf[INDEX_1], 
            u8DataBuf[INDEX_2], u8DataBuf[INDEX_3])) & PE_REQUEST_OPR_CUR_MASK) >> PE_REQUEST_OPR_CUR_START_POS);
	
    /* Get the current value of Requested Source PDO */
    u16SrcPDOCurrVal = ((gasCfgStatusData.sPerPortData[u8PortNum].u32aAdvertisedPDO[u8SinkReqObjPos-1]) & \
                     PE_REQUEST_MAX_CUR_MASK); 
    
    /* If Requested Max current is greater current value of Requested Source PDO or
        Requested object position is invalid, received request is invalid request */ 
    u8RetVal = (u16SinkReqCurrVal > u16SrcPDOCurrVal) ? DPM_INVALID_REQUEST : (((u8SinkReqObjPos<= FALSE) || \
               (u8SinkReqObjPos> gasCfgStatusData.sPerPortData[u8PortNum].u8AdvertisedPDOCnt))) ? \
                DPM_INVALID_REQUEST : (u8RaPresence == FALSE) ? DPM_VALID_REQUEST : \
                (u16SinkReqCurrVal > gasDPM[u8PortNum].u16MaxCurrSupportedin10mA) ? \
                DPM_INVALID_REQUEST : DPM_VALID_REQUEST;   
    
    /* If request is valid set the Negotiated PDO as requested */
    if(u8RetVal == DPM_VALID_REQUEST)
    {
        gasDPM[u8PortNum].u32NegotiatedPDO = (gasCfgStatusData.sPerPortData[u8PortNum].u32aAdvertisedPDO[u8SinkReqObjPos-1]);
        gasDPM[u8PortNum].u8NegotiatedPDOIndex = u8SinkReqObjPos;
        gasCfgStatusData.sPerPortData[u8PortNum].u32RDO = MAKE_UINT32_FROM_BYTES(u8DataBuf[INDEX_0], u8DataBuf[INDEX_1], 
                                        u8DataBuf[INDEX_2], u8DataBuf[INDEX_3]);
        gasCfgStatusData.sPerPortData[u8PortNum].u16NegoVoltageIn10mA = u16SrcPDOCurrVal; 
        gasCfgStatusData.sPerPortData[u8PortNum].u16NegoVoltageIn50mV = \
                ((DPM_GET_VOLTAGE_FROM_PDO_MILLI_V(gasDPM[u8PortNum].u32NegotiatedPDO)) / DPM_PDO_VOLTAGE_UNIT);
        /* Update the allocated power in terms of 250mW */
        gasCfgStatusData.sPerPortData[u8PortNum].u16AllocatedPowerIn250mW =    
                ((gasCfgStatusData.sPerPortData[u8PortNum].u16NegoVoltageIn50mV * gasCfgStatusData.sPerPortData[u8PortNum].u16NegoVoltageIn10mA) / 
                (DPM_PDO_VOLTAGE_UNIT * DPM_PDO_CURRENT_UNIT)); 
        DEBUG_PRINT_PORT_STR (u8PortNum,"DPM-PE: Requested is Valid \r\n");
    }

    return u8RetVal;
}

/* Reset the current value in PDO */
UINT32 DPM_CurrentCutDown (UINT32 u32PDO)
{
    /* If PDO max current greater than E-Cable supported current, reset the current value */
    if((u32PDO & PE_MAX_CURR_MASK) > DPM_CABLE_CURR_3A_UNIT)
    {
        u32PDO &= ~PE_MAX_CURR_MASK;
        u32PDO |= DPM_CABLE_CURR_3A_UNIT;
    }
    
    return u32PDO; 
}

/* Copy the Source capabilities */
void DPM_ChangeCapabilities (UINT8 u8PortNum, UINT32* pu32DataObj, UINT32 *pu32SrcCaps, UINT8 u8pSrcPDOCnt)
{
    
    /* The attached USB-C cable does not support the locally defined Source PDOs  */   
    gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus |= 
                                        DPM_PORT_CABLE_REDUCED_SRC_CAPABILITIES_STATUS;        

    for (UINT8 u8PDOindex = 0; u8PDOindex < u8pSrcPDOCnt; u8PDOindex++)
    {   
        /* Reset the current value to E-Cable supported current */
        pu32DataObj[u8PDOindex] = DPM_CurrentCutDown (pu32SrcCaps[u8PDOindex]);
    } 
}

/* Get the source capabilities from the port configuration structure */
void DPM_Get_Source_Capabilities(UINT8 u8PortNum, UINT8* u8pSrcPDOCnt, UINT32* pu32DataObj)
{   
  
    UINT8 u8RaPresence = SET_TO_ZERO;
	UINT32 *pu32SrcCap;
	/* Get the source PDO count */
    if (gasCfgStatusData.sPerPortData[u8PortNum].u8NewPDOSelect)
    {
        *u8pSrcPDOCnt = gasCfgStatusData.sPerPortData[u8PortNum].u8NewPDOCnt;
   
        pu32SrcCap = (UINT32 *)&gasCfgStatusData.sPerPortData[u8PortNum].u32aNewPDO[0];                        
    }
    else
    {
        *u8pSrcPDOCnt = gasCfgStatusData.sPerPortData[u8PortNum].u8SourcePDOCnt;
   
        pu32SrcCap = (UINT32 *)&gasCfgStatusData.sPerPortData[u8PortNum].u32aSourcePDO[0];        
    }
    
    DPM_GetPoweredCablePresence(u8PortNum, &u8RaPresence);
   
    /* E-Cable presents */
    if((TRUE == u8RaPresence))
    {
        /* If E-Cable max current is 5A, pass the capabilities without change */
        if(gasDPM[u8PortNum].u16MaxCurrSupportedin10mA == DPM_CABLE_CURR_5A_UNIT)
        {
            /* The attached USB-C cable supports the locally-defined Source PDOs */
            gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus &= 
                                        ~(DPM_PORT_CABLE_REDUCED_SRC_CAPABILITIES_STATUS);

            (void)MCHP_PSF_HOOK_MEMCPY(&pu32DataObj[INDEX_0], &pu32SrcCap[INDEX_0],\
                    ((*u8pSrcPDOCnt) * 4));
        }
        /* If E-Cable max current is 3A and PDO current value is more than 3A, 
            reset the current value of PDOs */
        else
        {
            DPM_ChangeCapabilities (u8PortNum, &pu32DataObj[INDEX_0], &pu32SrcCap[INDEX_0], *u8pSrcPDOCnt);
        }
          
    }   
    else
    {
        DPM_ChangeCapabilities (u8PortNum, pu32DataObj, &pu32SrcCap[0],*u8pSrcPDOCnt);  
    }     
}

void DPM_ResetNewPDOParameters(UINT8 u8PortNum)
{
    gasCfgStatusData.sPerPortData[u8PortNum].u8NewPDOSelect = FALSE; 
    
    gasCfgStatusData.sPerPortData[u8PortNum].u8NewPDOCnt = RESET_TO_ZERO; 
        
    for (UINT8 u8PDOIndex = INDEX_0; u8PDOIndex < CFG_MAX_PDO_COUNT; u8PDOIndex++)
    {
        gasCfgStatusData.sPerPortData[u8PortNum].u32aNewPDO[u8PDOIndex] = RESET_TO_ZERO; 
    }    
}

void DPM_UpdateAdvertisedPDOParam(UINT8 u8PortNum)
{
    if (gasCfgStatusData.sPerPortData[u8PortNum].u8NewPDOSelect)
    {
        /* Update Advertised PDO Count */
        gasCfgStatusData.sPerPortData[u8PortNum].u8AdvertisedPDOCnt = \
                            gasCfgStatusData.sPerPortData[u8PortNum].u8NewPDOCnt;

       /* Update Advertised PDO Registers with New PDOs if NewPDOSlct is enabled. */
        (void)MCHP_PSF_HOOK_MEMCPY(gasCfgStatusData.sPerPortData[u8PortNum].u32aAdvertisedPDO, 
            gasCfgStatusData.sPerPortData[u8PortNum].u32aNewPDO, (gasCfgStatusData.sPerPortData[u8PortNum].u8NewPDOCnt * 4));            
    }
    else
    {
        /* Update Advertised PDO Count */
        gasCfgStatusData.sPerPortData[u8PortNum].u8AdvertisedPDOCnt = \
                            gasCfgStatusData.sPerPortData[u8PortNum].u8SourcePDOCnt;

        /* Update Advertised PDO Registers with Default Source PDOs if NewPDOSlct is not enabled. */
        (void)MCHP_PSF_HOOK_MEMCPY(gasCfgStatusData.sPerPortData[u8PortNum].u32aAdvertisedPDO, 
            gasCfgStatusData.sPerPortData[u8PortNum].u32aSourcePDO, (gasCfgStatusData.sPerPortData[u8PortNum].u8SourcePDOCnt * 4));           
    }

    /* Update the Port Connection Status register by comparing the Fixed and 
       Advertised Source PDOs */
    if (0 == DPM_ComparePDOs(u8PortNum))
    {
        /* The advertised PDOs are equivalent to the default configured values */
        gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus &= 
                            ~(DPM_PORT_PD_BAL_REDUCED_SRC_CAPABILITIES_STATUS);         
    }   
    else
    {
        /* The advertised PDOs have been reduced from default configured values */
        gasCfgStatusData.sPerPortData[u8PortNum].u32PortConnectStatus |= 
                            DPM_PORT_PD_BAL_REDUCED_SRC_CAPABILITIES_STATUS;                 
    }    
}

UINT8 DPM_ComparePDOs(UINT8 u8PortNum)
{
    return MCHP_PSF_HOOK_MEMCMP(&gasCfgStatusData.sPerPortData[u8PortNum].u32aSourcePDO[INDEX_0],
                    &gasCfgStatusData.sPerPortData[u8PortNum].u32aAdvertisedPDO[INDEX_0], 
                    ((MAX(gasCfgStatusData.sPerPortData[u8PortNum].u8SourcePDOCnt, 
                        gasCfgStatusData.sPerPortData[u8PortNum].u8AdvertisedPDOCnt)) * 4)); 
}

void DPM_StoreSinkCapabilities(UINT8 u8PortNum, UINT16 u16Header, UINT32* u32DataBuf)
{   
    /* Store the count in Partner PDO Count. */
    gasCfgStatusData.sPerPortData[u8PortNum].u8PartnerPDOCnt = PRL_GET_OBJECT_COUNT(u16Header); 
    
    for (UINT8 u8PDOIndex = INDEX_0; u8PDOIndex < gasCfgStatusData.sPerPortData[u8PortNum].u8PartnerPDOCnt; u8PDOIndex++)
    {
      gasCfgStatusData.sPerPortData[u8PortNum].u32aPartnerPDO[u8PDOIndex] = u32DataBuf[u8PDOIndex];   
    }    
}
#endif /*INCLUDE_PD_SOURCE*/  

/*********************************DPM VDM Cable APIs**************************************/
UINT8 DPM_StoreVDMECableData(UINT8 u8PortNum, UINT8 u8SOPType, UINT16 u16Header, UINT32* u32DataBuf)
{
    UINT32 u32ProductTypeVDO;
    UINT8 u8RetVal = FALSE;
    UINT8 u8CurVal;
    
    /* Get the CMD type from received VDM */
    u8RetVal = DPM_VDM_GET_CMD_TYPE(u32DataBuf[DPM_VDM_HEADER_POS]);
    
    /* if Data object is one, received message is NAK */
    if(u8RetVal == PE_VDM_NAK || u8RetVal == PE_VDM_BUSY)
    {
        u8RetVal = PE_VDM_NAK;
    }
    else
    {
        /* Get the product VDO from the received message */
        u32ProductTypeVDO = u32DataBuf[DPM_VMD_PRODUCT_TYPE_VDO_POS];               
       
        /* Get the Cable supported current value */
        u8CurVal = DPM_GET_CABLE_CUR_VAL(u32ProductTypeVDO);
        
        /* Setting E-Cable Max Current Value */
        if(u8CurVal == DPM_CABLE_CURR_3A)
        {
            gasDPM[u8PortNum].u16MaxCurrSupportedin10mA = DPM_CABLE_CURR_3A_UNIT;
        }
        
        else if(u8CurVal == DPM_CABLE_CURR_5A)
        {
            gasDPM[u8PortNum].u16MaxCurrSupportedin10mA = DPM_CABLE_CURR_5A_UNIT;
        }
        
        else
        {
           /* Do nothing */ 
        }
        
        /* Received message is ACK */
        u8RetVal = PE_VDM_ACK;
    }
    
    return u8RetVal;
}
/*******************************************************************************/
/*****************************DPM API that access Policy Engine************/
UINT8 DPM_IsHardResetInProgress(UINT8 u8PortNum)
{
    UINT8 u8HardResetProgressStatus = ((gasPolicy_Engine[u8PortNum].u8PEPortSts & \
                                        PE_HARDRESET_PROGRESS_MASK) >> PE_HARDRESET_PROGRESS_POS);
    return u8HardResetProgressStatus;

}
/******************************************************************************/

#if (TRUE == INCLUDE_PD_SINK)
/****************************** DPM Sink related APIs*****************************************/
void DPM_Get_Sink_Capabilities(UINT8 u8PortNum,UINT8 *u8pSinkPDOCnt, UINT32 * pu32DataObj)
{   
    /* Get Sink Capability from Port Configuration Data Structure */
    *u8pSinkPDOCnt = gasCfgStatusData.sPerPortData[u8PortNum].u8SinkPDOCnt;
    
        (void)MCHP_PSF_HOOK_MEMCPY ( pu32DataObj, gasCfgStatusData.sPerPortData[u8PortNum].u32aSinkPDO, \
                            (gasCfgStatusData.sPerPortData[u8PortNum].u8SinkPDOCnt * 4));
}

void DPM_CalculateAndSortPower(UINT8 u8PDOCount, UINT32 *pu32CapsPayload, UINT8 u8Power[][2])
{
    UINT8 u8PowerSwap = 0;
    UINT8 u8PowerSwapIndex = 0;
    UINT8 u8PowerIndex;
    UINT8 u8PDOIndex;
    UINT32 u32PDO;
  
    /* Calculating and storing src power from minimum to maximum */
    for(u8PDOIndex = 0; u8PDOIndex < u8PDOCount; u8PDOIndex++)
    {
        u32PDO = pu32CapsPayload[u8PDOIndex];
        
        u8Power[u8PDOIndex][0] = (UINT8)((float)(DPM_GET_PDO_VOLTAGE(u32PDO)*50/(float)1000) *\
                                            ((float)((float)(DPM_GET_PDO_CURRENT(u32PDO) * 10) /(float)1000)));
 
        u8Power[u8PDOIndex][1] = u8PDOIndex;
    }
    
    for(u8PDOIndex = 0; u8PDOIndex < u8PDOCount; u8PDOIndex++)
    {          
        for(u8PowerIndex = 0; u8PowerIndex < (u8PDOCount - u8PDOIndex - 1); u8PowerIndex++)
        {
            if(u8Power[u8PowerIndex][0] <= u8Power[u8PowerIndex + 1][0])
            {
               u8PowerSwap = u8Power[u8PowerIndex][0];
               u8PowerSwapIndex = u8Power[u8PowerIndex][1];
               u8Power[u8PowerIndex][0] = u8Power[u8PowerIndex + 1][0];
               u8Power[u8PowerIndex][1] = u8Power[u8PowerIndex + 1][1];
               u8Power[u8PowerIndex + 1][0] = u8PowerSwap; 
               u8Power[u8PowerIndex + 1][1] = u8PowerSwapIndex;
            }
        }
    }
  
}

void DPM_Evaluate_Received_Src_caps(UINT8 u8PortNum ,UINT16 u16RecvdSrcCapsHeader,
                                     UINT32 *pu32RecvdSrcCapsPayload)
{
    UINT8 u8SrcPower[7][2] = {0};
    UINT8 u8SinkPower[7][2] = {0};
    UINT8 u8SrcIndex = 0;
    UINT8 u8SinkIndex = 0;
    UINT8 u8CapMismatch = FALSE;
    //UINT32 u32SinkSelectedPDO;    
    /*PDO Count of the sink*/
	UINT8 u8SinkPDOCnt = gasCfgStatusData.sPerPortData[u8PortNum].u8SinkPDOCnt;
    /*PDO Count of the source derived from received src caps*/
	UINT8 u8Recevd_SrcPDOCnt =  PRL_GET_OBJECT_COUNT(u16RecvdSrcCapsHeader);
    UINT32 u32RcvdSrcPDO;
    //UINT32 u32PDO;
    UINT8 u8SrcPDOIndex;
    UINT8 u8SinkPDOIndex;
    UINT32 u32SinkPDO;
    
    (void)MCHP_PSF_HOOK_MEMCPY(gasCfgStatusData.sPerPortData[u8PortNum].u32aPartnerPDO, 
            gasCfgStatusData.sPerPortData[u8PortNum].u32aSourcePDO, (gasCfgStatusData.sPerPortData[u8PortNum].u8SourcePDOCnt * 4));
        
    /* Calculate and sort the power of Sink PDOs */
    DPM_CalculateAndSortPower(u8SinkPDOCnt, &gasCfgStatusData.sPerPortData[u8PortNum].u32aSinkPDO[0], u8SinkPower);
    
    /* Calculate and sort the received source PDOs power */
    DPM_CalculateAndSortPower(u8Recevd_SrcPDOCnt, pu32RecvdSrcCapsPayload, u8SrcPower);
    
    /* Compare Maximum power sink PDO to received source PDOs */
    u8SinkPDOIndex = u8SinkPower[0][1];
    u32SinkPDO = gasCfgStatusData.sPerPortData[u8PortNum].u32aSinkPDO[u8SinkPDOIndex];
    
    for(u8SrcIndex = 0; u8SrcIndex < u8Recevd_SrcPDOCnt; u8SrcIndex++)
    {
        if(u8SrcPower[u8SrcIndex][0] >= u8SinkPower[0][0])
        {
            u8SrcPDOIndex = u8SrcPower[u8SrcIndex][1];
            u32RcvdSrcPDO = pu32RecvdSrcCapsPayload[u8SrcPDOIndex];
 
            
            if((DPM_GET_PDO_VOLTAGE(u32RcvdSrcPDO) == DPM_GET_PDO_VOLTAGE(u32SinkPDO)) && \
                (DPM_GET_PDO_CURRENT(u32RcvdSrcPDO)) >= DPM_GET_PDO_CURRENT(u32SinkPDO))
            {
                /* Form request message */
                gasCfgStatusData.sPerPortData[u8PortNum].u32RDO = DPM_FORM_DATA_REQUEST((u8SrcPDOIndex + 1),\
                                                u8CapMismatch,DPM_GET_PDO_USB_COMM_CAP(u32SinkPDO),\
                                                DPM_GET_PDO_CURRENT(u32SinkPDO),\
                                                DPM_GET_PDO_CURRENT(u32SinkPDO));
                
                gasDPM[u8PortNum].u16MaxCurrSupportedin10mA = DPM_GET_PDO_CURRENT(u32SinkPDO);
                
                /*Updating the globals with Sink PDO selected */
                gasDPM[u8PortNum].u32NegotiatedPDO = u32SinkPDO;
                
                /*Updating the globals for Sink */
                 gasDPM[u8PortNum].u8NegotiatedPDOIndex = u8SinkPDOIndex+1;
                 
                /*VBUS Threshold are configured for the requested PDO*/
                DPM_SetPortPower (u8PortNum);
                
                return;
            }
        }
    }
    u8CapMismatch = TRUE;
    
    for(u8SrcIndex = 0; u8SrcIndex < u8Recevd_SrcPDOCnt; u8SrcIndex++)
    {
        u8SrcPDOIndex = u8SrcPower[u8SrcIndex][1];
        u32RcvdSrcPDO = pu32RecvdSrcCapsPayload[u8SrcPDOIndex];
        
        for(u8SinkIndex = 0; u8SinkIndex < u8SinkPDOCnt; u8SinkIndex++)
        {
            u8SinkPDOIndex = u8SrcPower[u8SinkIndex][1];
            u32SinkPDO = gasCfgStatusData.sPerPortData[u8PortNum].u32aSinkPDO[u8SinkPDOIndex];
            
            if((DPM_GET_PDO_VOLTAGE(u32RcvdSrcPDO)) == DPM_GET_PDO_VOLTAGE(u32SinkPDO))
            {
                /* Form Request message with Capability Mismatch*/
                gasCfgStatusData.sPerPortData[u8PortNum].u32RDO = DPM_FORM_DATA_REQUEST((u8SrcPDOIndex + 1),\
                                                u8CapMismatch,DPM_GET_PDO_USB_COMM_CAP(u32SinkPDO),\
                                                DPM_GET_PDO_CURRENT(u32RcvdSrcPDO),\
                                                DPM_GET_PDO_CURRENT(u32RcvdSrcPDO));
                
                gasDPM[u8PortNum].u16MaxCurrSupportedin10mA = DPM_GET_PDO_CURRENT(u32RcvdSrcPDO);
                
                /*Updating the globals with Sink PDO selected */
                gasDPM[u8PortNum].u32NegotiatedPDO = u32RcvdSrcPDO;
                /*Updating the globals with Sink PDO index selected*/
                gasDPM[u8PortNum].u8NegotiatedPDOIndex = (u8SinkPDOIndex+1);
                /*VBUS Threshold are configured for the requested PDO*/
                DPM_SetPortPower (u8PortNum);
                
                return;
            }
        }
    }
          
}
#endif

/********************************VCONN Related APIs**********************************************/
UINT8 DPM_Evaluate_VCONN_Swap(UINT8 u8PortNum)
{
    /*As of now, Accept the VCONN Swap without any restriction*/
    return TRUE;   
}
UINT8 DPM_IsPort_VCONN_Source(UINT8 u8PortNum)
{ 
    UINT8 u8IsVCONNSrc;
    if(gasTypeCcontrol[u8PortNum].u8IntStsISR & TYPEC_VCONN_SOURCE_MASK)
    {
        u8IsVCONNSrc =TRUE;
    }
    else
    {
        u8IsVCONNSrc =FALSE;
    }
    return u8IsVCONNSrc;
}

/********************************Power Fault API ******************************/

#if (TRUE == INCLUDE_POWER_FAULT_HANDLING)
static void DPM_ClearPowerfaultFlags(UINT8 u8PortNum)
{
    /*ISR flag is cleared by disabling the interrupt*/
    MCHP_PSF_HOOK_DISABLE_GLOBAL_INTERRUPT();
    gasDPM[u8PortNum].u8PowerFaultISR = SET_TO_ZERO;
    MCHP_PSF_HOOK_ENABLE_GLOBAL_INTERRUPT();
}
void DPM_PowerFaultHandler(UINT8 u8PortNum)
{
  	/* Incase detach reset the Power Fault handling variables*/
    if (((gasTypeCcontrol[u8PortNum].u8TypeCState == TYPEC_UNATTACHED_SRC) &&
		    (gasTypeCcontrol[u8PortNum].u8TypeCSubState == TYPEC_UNATTACHED_SRC_INIT_SS))||
				 ((gasTypeCcontrol[u8PortNum].u8TypeCState == TYPEC_UNATTACHED_SNK) &&
				   (gasTypeCcontrol[u8PortNum].u8TypeCSubState == TYPEC_UNATTACHED_SNK_INIT_SS)))
    {
		/* Enable Fault PIO to detect OCS as it would have been disabled as part of
         Power fault handling*/
        UPD_EnableFaultIn(u8PortNum);
		
		/* Reset Wait for HardReset Complete bit*/
        gasDPM[u8PortNum].u8HRCompleteWait = SET_TO_ZERO;
		
		/* Kill the timer*/
        PDTimer_Kill (gasDPM[u8PortNum].u8VBUSPowerGoodTmrID);
		
		/*Setting the u8VBUSPowerGoodTmrID to MAX_CONCURRENT_TIMERS to indicate that
    	TimerID does not hold any valid timer IDs anymore*/
        gasDPM[u8PortNum].u8VBUSPowerGoodTmrID = MAX_CONCURRENT_TIMERS;
		
		/* Setting the power fault count to Zero */
        if(gasDPM[u8PortNum].u8TypeCErrRecFlag == SET_TO_ZERO)
        {
            gasDPM[u8PortNum].u8VBUSPowerFaultCount = SET_TO_ZERO;
        }
        
        gasDPM[u8PortNum].u8TypeCErrRecFlag = SET_TO_ZERO;
        	
        
        /*******Resetting the VCONN OCS related variables************/
        
        /*Setting VCONNGoodtoSupply flag as true*/        
        gasDPM[u8PortNum].u8VCONNGoodtoSupply = TRUE;
        
        /* Killing the VCONN power good timer*/
        PDTimer_Kill (gasDPM[u8PortNum].u8VCONNPowerGoodTmrID);
        
        /*Setting the u8VCONNPowerGoodTmrID to MAX_CONCURRENT_TIMERS to indicate that
    	TimerID does not hold any valid timer IDs anymore*/
        gasDPM[u8PortNum].u8VCONNPowerGoodTmrID = MAX_CONCURRENT_TIMERS;
        
        /*Resetting the VCONN OCS fault count to Zero */
        gasDPM[u8PortNum].u8VCONNPowerFaultCount = SET_TO_ZERO;
        
        /*ISR flag for OVP,UVP,OCP,VCONN OCS is cleared by disabling the interrupt*/
        MCHP_PSF_HOOK_DISABLE_GLOBAL_INTERRUPT();
        gasDPM[u8PortNum].u8PowerFaultISR = SET_TO_ZERO;
        MCHP_PSF_HOOK_ENABLE_GLOBAL_INTERRUPT();
        
    }	
    if(gasDPM[u8PortNum].u8HRCompleteWait) 
    { 
        if((gasPolicy_Engine[u8PortNum].ePESubState == ePE_SRC_TRANSITION_TO_DEFAULT_POWER_ON_SS) ||
				 (gasPolicy_Engine[u8PortNum].ePEState == ePE_SNK_STARTUP))
        {
            if(gasDPM[u8PortNum].u8VCONNPowerFaultCount >= (gasCfgStatusData.sPerPortData[u8PortNum].u8VCONNMaxFaultCnt))
            {            
                /*Setting the VCONN Good to Supply Flag as False*/
                gasDPM[u8PortNum].u8VCONNGoodtoSupply = FALSE;
            }
            if (gasDPM[u8PortNum].u8VBUSPowerFaultCount >= (gasCfgStatusData.sPerPortData[u8PortNum].u8VBUSMaxFaultCnt))
            {
				/* Disable the receiver*/
                PRL_EnableRx (u8PortNum, FALSE);
				
				/* kill all the timers*/
                PDTimer_KillPortTimers (u8PortNum);
				
				/* set the fault count to zero */
                gasDPM[u8PortNum].u8VBUSPowerFaultCount = SET_TO_ZERO;
				
                DEBUG_PRINT_PORT_STR (u8PortNum, "PWR_FAULT: u8HRCompleteWait Reseted ");
				
                if (DPM_GET_CURRENT_POWER_ROLE(u8PortNum) == PD_ROLE_SOURCE)
                {			
					/* Assign an idle state wait for detach*/
                    gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ATTACHED_SRC_IDLE_SS;
                }
                else
                { 
					/* Assign an idle state wait for detach*/
                    gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ATTACHED_SNK_IDLE_SS;
                }
				/* Assign an idle state wait for detach*/
                gasPolicy_Engine[u8PortNum].ePEState = ePE_INVALIDSTATE;
                DEBUG_PRINT_PORT_STR (u8PortNum, "PWR_FAULT: Entered SRC/SNK Powered OFF state");
            }
            else
            {
                /* Enable Fault PIO to detect OCS as it would have been disabled as part of
                    Power fault handling*/
                UPD_EnableFaultIn(u8PortNum);
            }
			/* Reset Wait for HardReset Complete bit*/
            gasDPM[u8PortNum].u8HRCompleteWait = SET_TO_ZERO;
			
        }
    }
    if (gasDPM[u8PortNum].u8PowerFaultISR)
    {
        DEBUG_PRINT_PORT_STR(u8PortNum, "DPM Fault Handling");
        /*If VCONN OCS is present , kill the VCONN power good timer*/
        if(gasDPM[u8PortNum].u8PowerFaultISR & DPM_POWER_FAULT_VCONN_OCS)
        {
            if(FALSE == DPM_NotifyClient(u8PortNum, eMCHP_PSF_VCONN_PWR_FAULT))
            {
                /*Clear the Power fault flag and return*/
                DPM_ClearPowerfaultFlags(u8PortNum);
                return;
            }
            /*Kill the VCONN Power fault timer*/
            PDTimer_Kill (gasDPM[u8PortNum].u8VCONNPowerGoodTmrID);
        
             /*Setting the u8VCONNPowerGoodTmrID to MAX_CONCURRENT_TIMERS to indicate that
            TimerID does not hold any valid timer IDs anymore*/
            gasDPM[u8PortNum].u8VCONNPowerGoodTmrID = MAX_CONCURRENT_TIMERS;
			
            DEBUG_PRINT_PORT_STR (u8PortNum, "PWR_FAULT: VCONN Power Fault");
        }
        if(gasDPM[u8PortNum].u8PowerFaultISR & ~DPM_POWER_FAULT_VCONN_OCS)
        { 
            if(FALSE == DPM_NotifyClient(u8PortNum, eMCHP_PSF_VBUS_PWR_FAULT))
            {
                /*Clear the Power fault flag and return*/
                DPM_ClearPowerfaultFlags(u8PortNum);
                return;
            }
             /*Toggle DC_DC EN on VBUS fault to reset the DC-DC controller*/
            PWRCTRL_ConfigDCDCEn(u8PortNum, FALSE);    
            
            #if (TRUE == INCLUDE_UPD_PIO_OVERRIDE_SUPPORT)
            /*Clear PIO override enable*/
            UPD_RegByteClearBit (u8PortNum, UPD_PIO_OVR_EN,  UPD_PIO_OVR_2);
            #endif
            
            /* Kill Power Good Timer */
            PDTimer_Kill (gasDPM[u8PortNum].u8VBUSPowerGoodTmrID);
        
            /*Setting the u8VBUSPowerGoodTmrID to MAX_CONCURRENT_TIMERS to indicate that
            TimerID does not hold any valid timer IDs anymore*/
            gasDPM[u8PortNum].u8VBUSPowerGoodTmrID = MAX_CONCURRENT_TIMERS;
			
            DEBUG_PRINT_PORT_STR (u8PortNum, "PWR_FAULT: VBUS Power Fault");
        }
        if(PE_GET_PD_CONTRACT(u8PortNum) == PE_IMPLICIT_CONTRACT)
        {
			/* Set it to Type C Error Recovery */
            gasTypeCcontrol[u8PortNum].u8TypeCState = TYPEC_ERROR_RECOVERY;
            gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ERROR_RECOVERY_ENTRY_SS;
            
            gasDPM[u8PortNum].u8TypeCErrRecFlag = 0x01;
						
            /*Increment the fault count*/
            gasDPM[u8PortNum].u8VBUSPowerFaultCount++;
            
            if (gasDPM[u8PortNum].u8VBUSPowerFaultCount >= CFG_MAX_VBUS_POWER_FAULT_COUNT)
            {
				/* Disable the receiver*/
                //PRL_EnableRx (u8PortNum, FALSE);
				
				/* kill all the timers*/
                PDTimer_KillPortTimers (u8PortNum);
				
				/* set the fault count to zero */
                gasDPM[u8PortNum].u8VBUSPowerFaultCount = SET_TO_ZERO;
				
                DEBUG_PRINT_PORT_STR (u8PortNum, "PWR_FAULT: u8HRCompleteWait Resetted ");
				
                if (DPM_GET_CURRENT_POWER_ROLE(u8PortNum) == PD_ROLE_SOURCE)
                {			
					/* Assign an idle state wait for detach*/
                    gasTypeCcontrol[u8PortNum].u8TypeCState = TYPEC_ATTACHED_SRC;
                    gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ATTACHED_SRC_IDLE_SS;
        
                }
                else
                { 
					/* Assign an idle state wait for detach*/
                    gasTypeCcontrol[u8PortNum].u8TypeCState = TYPEC_ATTACHED_SNK;
                    gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ATTACHED_SNK_IDLE_SS;
                }
				/* Assign an idle state wait for detach*/
                //gasPolicy_Engine[u8PortNum].ePEState = ePE_INVALIDSTATE;
                DEBUG_PRINT_PORT_STR (u8PortNum, "PWR_FAULT: Entered SRC/SNK Powered OFF state");
                
                gasDPM[u8PortNum].u8TypeCErrRecFlag = 0x00;
            }
			  
			/* Assign an idle state wait for detach*/
            //gasPolicy_Engine[u8PortNum].ePEState = ePE_INVALIDSTATE;
        }
        else
        {          
            if(gasDPM[u8PortNum].u8PowerFaultISR & DPM_POWER_FAULT_VCONN_OCS)
            {           
                /*Increment the VCONN fault count*/
                gasDPM[u8PortNum].u8VCONNPowerFaultCount++;
                
                /*CC comparator will off once VCONN OCS is detected for implicit contract it is 
                 enabled as part of Type C error recovery. For explicit contract it is enabled here*/
                /*Enabling the CC Sampling on CC1 and CC2 lines*/
                TypeC_ConfigCCComp (u8PortNum, TYPEC_CC_COMP_CTL_CC1_CC2);
            }
            if(gasDPM[u8PortNum].u8PowerFaultISR & ~DPM_POWER_FAULT_VCONN_OCS)
            {
                /*Increment the fault count*/
                gasDPM[u8PortNum].u8VBUSPowerFaultCount++;            
            }
			/* Send Hard reset*/
            PE_SendHardResetMsg(u8PortNum);
			
			/* Set Wait for HardReset Complete bit*/
            gasDPM[u8PortNum].u8HRCompleteWait = gasDPM[u8PortNum].u8PowerFaultISR;
        }
        
        /* Enable DC_DC_EN to drive power*/        
        PWRCTRL_ConfigDCDCEn(u8PortNum, TRUE); 

        #if (TRUE == INCLUDE_UPD_PIO_OVERRIDE_SUPPORT)
        /*Enable PIO override enable*/
        UPD_RegByteSetBit (u8PortNum, UPD_PIO_OVR_EN,  UPD_PIO_OVR_2);
        #endif
            
		/*Clear the Power fault flag*/
        DPM_ClearPowerfaultFlags(u8PortNum);
    }
}
void DPM_VCONNPowerGood_TimerCB (UINT8 u8PortNum, UINT8 u8DummyVariable)
{
	/* Set the timer Id to Max Value*/
 	gasDPM[u8PortNum].u8VCONNPowerGoodTmrID = MAX_CONCURRENT_TIMERS;	
	/* Resetting the VCONN fault Count*/
	gasDPM[u8PortNum].u8VCONNPowerFaultCount = RESET_TO_ZERO;
}
#endif 

/*************************************VBUS & VCONN on/off Timer APIS*********************************/
void DPM_VBUSOnOffTimerCB (UINT8 u8PortNum, UINT8 u8DummyVariable)
{   
    gasTypeCcontrol[u8PortNum].u8TypeCState = TYPEC_ERROR_RECOVERY;
    gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ERROR_RECOVERY_ENTRY_SS;
    
    gasPolicy_Engine[u8PortNum].ePEState = ePE_INVALIDSTATE;
    gasPolicy_Engine[u8PortNum].ePESubState = ePE_INVALIDSUBSTATE;
    
    gasPolicy_Engine[u8PortNum].u8PETimerID = MAX_CONCURRENT_TIMERS;
}
void DPM_SrcReadyTimerCB (UINT8 u8PortNum, UINT8 u8DummyVariable)
{
    if(gasPolicy_Engine[u8PortNum].u8PEPortSts & PE_EXPLICIT_CONTRACT)
    {
        gasPolicy_Engine[u8PortNum].ePEState = ePE_SRC_HARD_RESET;
        gasPolicy_Engine[u8PortNum].ePESubState = ePE_SRC_HARD_RESET_ENTRY_SS;
    }
    
    else
    {
        DPM_VBUSOnOffTimerCB ( u8PortNum, u8DummyVariable);
    }
    
    gasPolicy_Engine[u8PortNum].u8PETimerID = MAX_CONCURRENT_TIMERS;
}

void DPM_VCONNONTimerErrorCB (UINT8 u8PortNum , UINT8 u8DummyVariable)
{ 
    gasTypeCcontrol[u8PortNum].u8PortSts &= ~TYPEC_VCONN_ON_REQ_MASK;
    gasPolicy_Engine[u8PortNum].u8PETimerID = MAX_CONCURRENT_TIMERS;
    
    if(gasDPM[u8PortNum].u8VCONNErrCounter > (gasCfgStatusData.sPerPortData[u8PortNum].u8VCONNMaxFaultCnt))
    {      
        /*Disable the receiver*/
        PRL_EnableRx (u8PortNum, FALSE);
        
        /*Kill all the Port timers*/
        PDTimer_KillPortTimers (u8PortNum);
        
        /*Disable VCONN by switching off the VCONN FETS which was enabled previously*/
        TypeC_EnabDisVCONN (u8PortNum, TYPEC_VCONN_DISABLE);
        
        if (DPM_GET_CURRENT_POWER_ROLE(u8PortNum) == PD_ROLE_SOURCE)
        {		          
            /*Disable VBUS by driving to vSafe0V if port role is a source*/
            DPM_TypeCVBus5VOnOff(u8PortNum, DPM_VBUS_OFF);
        
            /*Assign an idle state to wait for detach*/
            gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ATTACHED_SRC_IDLE_SS;
            
            DEBUG_PRINT_PORT_STR(u8PortNum,"VCONN_ON_ERROR: Entered SRC Powered OFF state");
        }
        else
        { 
            /*Assign an idle state to wait for detach*/
            gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ATTACHED_SNK_IDLE_SS;
            
            DEBUG_PRINT_PORT_STR(u8PortNum,"VCONN_ON_ERROR: Entered SNK Powered OFF state");
        }       
        gasPolicy_Engine[u8PortNum].ePEState = ePE_INVALIDSTATE;
        gasPolicy_Engine[u8PortNum].ePESubState = ePE_INVALIDSUBSTATE;
    }
    else
    {
        gasDPM[u8PortNum].u8VCONNErrCounter++;    
        PE_SendHardResetMsg(u8PortNum);    
    }    
}

void DPM_VCONNOFFErrorTimerCB (UINT8 u8PortNum , UINT8 u8DummyVariable)
{  
    /*Set it to Type C Error Recovery */
    gasTypeCcontrol[u8PortNum].u8TypeCState = TYPEC_ERROR_RECOVERY;
    gasTypeCcontrol[u8PortNum].u8TypeCSubState = TYPEC_ERROR_RECOVERY_ENTRY_SS;
    
    gasPolicy_Engine[u8PortNum].u8PETimerID = MAX_CONCURRENT_TIMERS;
    /* Assign an idle state wait for detach*/
    gasPolicy_Engine[u8PortNum].ePEState = ePE_INVALIDSTATE;
  
}

void DPM_ResetVCONNErrorCnt (UINT8 u8PortNum)
{  
    gasDPM[u8PortNum].u8VCONNErrCounter = SET_TO_ZERO;  
}
/*******************************************************************************/

/************************DPM Client Request******************************/

UINT8 DPM_HandleClientRequest(UINT8 u8PortNum,eMCHP_PSF_DPM_ClientRequest eDPMClientRequestType)
{
    UINT8 u8RetVal = TRUE;
    switch(eDPMClientRequestType)
    {
        case eMCHP_PSF_DPM_RENEGOTIATE:
        {
            if(TRUE == PE_IsPolicyEngineIdle(u8PortNum))
            {
                if (DPM_GET_CURRENT_POWER_ROLE(u8PortNum) == PD_ROLE_SOURCE)
                {
                    /**Send Source capability Policy Engine states are set*/
                    gasPolicy_Engine[u8PortNum].ePEState = ePE_SRC_SEND_CAPABILITIES;
                    gasPolicy_Engine[u8PortNum].ePESubState = ePE_SRC_SEND_CAP_ENTRY_SS;
                }
                else
                {
                    /*TBD for Sink*/
                }
            }
            else
            {
                u8RetVal = FALSE;
            }
            break;
        }
        case eMCHP_PSF_DPM_HANDLE_VBUS_FAULT:
        {
            /**VBUS OCS flag is set for DPM to handle the VBUS fault*/
            MCHP_PSF_HOOK_DISABLE_GLOBAL_INTERRUPT();
            gasDPM[u8PortNum].u8PowerFaultISR |= DPM_POWER_FAULT_VBUS_OCS;
            MCHP_PSF_HOOK_ENABLE_GLOBAL_INTERRUPT();
            break;
        }
        case eMCHP_PSF_DPM_GET_SNK_CAPS:
        {
            if(TRUE == (PE_IsPolicyEngineIdle(u8PortNum)) && 
                    (DPM_GET_CURRENT_POWER_ROLE(u8PortNum) == PD_ROLE_SOURCE))
            {
                gasPolicy_Engine[u8PortNum].ePEState = ePE_SRC_GET_SINK_CAP; 
                gasPolicy_Engine[u8PortNum].ePESubState = ePE_SRC_GET_SINK_CAP_ENTRY_SS;
            }
            else
            {
                u8RetVal = FALSE;
            }
            break;
        }
    }
    return u8RetVal;
} 

UINT8 DPM_NotifyClient(UINT8 u8PortNum, eMCHP_PSF_NOTIFICATION eDPMNotification)
{
    UINT8 u8Return = TRUE; 
    
#if (TRUE == INCLUDE_POWER_BALANCING)
    if (TRUE == IS_PB_ENABLED(u8PortNum))
    {
        u8Return = PB_HandleDPMEvents(u8PortNum, (UINT8)eDPMNotification);
    }
#endif
    
    /* DPM notifications that need to be handled by stack applications must
       be added here before calling the user function. */
    
    u8Return &= MCHP_PSF_NOTIFY_CALL_BACK(u8PortNum, (UINT8)eDPMNotification); 
     
    return u8Return;
}

