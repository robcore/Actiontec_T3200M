// <:copyright-BRCM:2013:DUAL/GPL:standard
// 
//    Copyright (c) 2013 Broadcom Corporation
//    All Rights Reserved
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2, as published by
// the Free Software Foundation (the "GPL").
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// 
// A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
// writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
// 
// :>
/*
 * ipsec object header file.
 * This header file is generated automatically. Do not edit!
 */
#ifndef _RDPA_AG_IPSEC_H_
#define _RDPA_AG_IPSEC_H_

/** \addtogroup ipsec
 * @{
 */


/** Get ipsec type handle.
 *
 * This handle should be passed to bdmf_new_and_set() function in
 * order to create an ipsec object.
 * \return ipsec type handle
 */
bdmf_type_handle rdpa_ipsec_drv(void);

/* ipsec: Attribute types */
typedef enum {
    rdpa_ipsec_attr_index = 0, /* index : R : number/4 : IPsec object index */
    rdpa_ipsec_attr_sa_table_ddr_addr = 1, /* sa_table_ddr_addr : MRI : number/4 : ddr sa table base address */
    rdpa_ipsec_attr_sa_entry_size = 2, /* sa_entry_size : MRI : number/2 : sa table entry size */
} rdpa_ipsec_attr_types;

extern int (*f_rdpa_ipsec_get)(bdmf_object_handle *pmo);

/** Get ipsec object.

 * This function returns ipsec object instance.
 * \param[out] ipsec_obj    Object handle
 * \return    0=OK or error <0
 */
int rdpa_ipsec_get(bdmf_object_handle *ipsec_obj);

/** Get ipsec/index attribute.
 *
 * Get IPsec object index.
 * \param[in]   mo_ ipsec object handle or mattr transaction handle
 * \param[out]  index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_ipsec_index_get(bdmf_object_handle mo_, bdmf_number *index_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_ipsec_attr_index, &_nn_);
    *index_ = (bdmf_number)_nn_;
    return _rc_;
}


/** Get ipsec/sa_table_ddr_addr attribute.
 *
 * Get ddr sa table base address.
 * \param[in]   mo_ ipsec object handle or mattr transaction handle
 * \param[out]  sa_table_ddr_addr_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_ipsec_sa_table_ddr_addr_get(bdmf_object_handle mo_, bdmf_number *sa_table_ddr_addr_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_ipsec_attr_sa_table_ddr_addr, &_nn_);
    *sa_table_ddr_addr_ = (bdmf_number)_nn_;
    return _rc_;
}


/** Set ipsec/sa_table_ddr_addr attribute.
 *
 * Set ddr sa table base address.
 * \param[in]   mo_ ipsec object handle or mattr transaction handle
 * \param[in]   sa_table_ddr_addr_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_ipsec_sa_table_ddr_addr_set(bdmf_object_handle mo_, bdmf_number sa_table_ddr_addr_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_ipsec_attr_sa_table_ddr_addr, sa_table_ddr_addr_);
}


/** Get ipsec/sa_entry_size attribute.
 *
 * Get sa table entry size.
 * \param[in]   mo_ ipsec object handle or mattr transaction handle
 * \param[out]  sa_entry_size_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_ipsec_sa_entry_size_get(bdmf_object_handle mo_, bdmf_number *sa_entry_size_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_ipsec_attr_sa_entry_size, &_nn_);
    *sa_entry_size_ = (bdmf_number)_nn_;
    return _rc_;
}


/** Set ipsec/sa_entry_size attribute.
 *
 * Set sa table entry size.
 * \param[in]   mo_ ipsec object handle or mattr transaction handle
 * \param[in]   sa_entry_size_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_ipsec_sa_entry_size_set(bdmf_object_handle mo_, bdmf_number sa_entry_size_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_ipsec_attr_sa_entry_size, sa_entry_size_);
}

/** @} end of ipsec Doxygen group */




#endif /* _RDPA_AG_IPSEC_H_ */
