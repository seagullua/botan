/*
* ECC Key implemenation
* (C) 2007 Manuel Hartl, FlexSecure GmbH
*          Falko Strenzke, FlexSecure GmbH
*     2008 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/ecc_key.h>
#include <botan/x509_key.h>
#include <botan/numthry.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>
#include <botan/secmem.h>
#include <botan/point_gfp.h>

namespace Botan {

/*
* EC_PublicKey
*/
void EC_PublicKey::affirm_init() const // virtual
   {
   if((mp_dom_pars.get() == 0) || (mp_public_point.get() == 0))
      throw Invalid_State("cannot use uninitialized EC_Key");
   }

const EC_Domain_Params& EC_PublicKey::domain_parameters() const
   {
   if(!mp_dom_pars.get())
      throw Invalid_State("EC_PublicKey::domain_parameters(): "
                          "ec domain parameters are not yet set");

   return *mp_dom_pars;
   }

const PointGFp& EC_PublicKey::public_point() const
   {
   if(!mp_public_point.get())
      throw Invalid_State("EC_PublicKey::public_point(): public point not set");

   return *mp_public_point;
   }

bool EC_PublicKey::domain_parameters_set()
   {
   return mp_dom_pars.get();
   }

void EC_PublicKey::X509_load_hook()
   {
   try
      {
      // the base point is checked to be on curve already when decoding it
      affirm_init();
      mp_public_point->check_invariants();
      }
   catch(Illegal_Point)
      {
      throw Decoding_Error("decoded public point was found not to lie on curve");
      }
   }

std::pair<AlgorithmIdentifier, MemoryVector<byte> >
EC_PublicKey::subject_public_key_info() const
   {
   this->affirm_init();

   SecureVector<byte> params =
      encode_der_ec_dompar(this->domain_parameters(), this->m_param_enc);

   AlgorithmIdentifier alg_id(this->get_oid(), params);
   MemoryVector<byte> key_bits = EC2OSP(*(this->mp_public_point), PointGFp::COMPRESSED);

   return std::make_pair(alg_id, key_bits);
   }

void EC_PublicKey::set_parameter_encoding(EC_dompar_enc type)
   {
   if((type != ENC_EXPLICIT) && (type != ENC_IMPLICITCA) && (type != ENC_OID))
      throw Invalid_Argument("Invalid encoding type for EC-key object specified");

   affirm_init();

   if((type == ENC_OID) && (mp_dom_pars->get_oid() == ""))
      throw Invalid_Argument("Invalid encoding type ENC_OID specified for "
                             "EC-key object whose corresponding domain "
                             "parameters are without oid");

   m_param_enc = type;
   }

/********************************
* EC_PrivateKey
********************************/
void EC_PrivateKey::affirm_init() const // virtual
   {
   if(m_private_value == 0)
      throw Invalid_State("cannot use EC_PrivateKey when private key is uninitialized");

   EC_PublicKey::affirm_init();
   }

const BigInt& EC_PrivateKey::private_value() const
   {
   if(m_private_value == 0)
      throw Invalid_State("cannot use EC_PrivateKey when private key is uninitialized");

   return m_private_value;
   }

/**
* EC_PrivateKey generator
**/
void EC_PrivateKey::generate_private_key(RandomNumberGenerator& rng)
   {
   if(mp_dom_pars.get() == 0)
      {
      throw Invalid_State("cannot generate private key when domain parameters are not set");
      }

   BigInt tmp_private_value(0);
   tmp_private_value = BigInt::random_integer(rng, 1, mp_dom_pars->get_order());
   mp_public_point = std::auto_ptr<PointGFp>( new PointGFp (mp_dom_pars->get_base_point()));
   mp_public_point->mult_this_secure(tmp_private_value,
                                     mp_dom_pars->get_order(),
                                     mp_dom_pars->get_order()-1);

   //assert(mp_public_point.get() != 0);
   tmp_private_value.swap(m_private_value);
   }

std::pair<AlgorithmIdentifier, SecureVector<byte> >
EC_PrivateKey::pkcs8_encoding() const
   {
   this->affirm_init();

   MemoryVector<byte> params =
      encode_der_ec_dompar(this->domain_parameters(), ENC_EXPLICIT);

   AlgorithmIdentifier alg_id(this->get_oid(), params);

   SecureVector<byte> octstr_secret =
      BigInt::encode_1363(this->m_private_value, this->m_private_value.bytes());

   SecureVector<byte> key_bits =
      DER_Encoder()
      .start_cons(SEQUENCE)
         .encode(BigInt(1))
         .encode(octstr_secret, OCTET_STRING)
      .end_cons()
      .get_contents();

   return std::make_pair(alg_id, key_bits);
   }

void EC_PrivateKey::PKCS8_load_hook(bool)
   {
   // we cannot use affirm_init() here because mp_public_point might still be null
   if(mp_dom_pars.get() == 0)
      throw Invalid_State("attempt to set public point for an uninitialized key");

   mp_public_point.reset(new PointGFp(m_private_value * mp_dom_pars->get_base_point()));
   mp_public_point->check_invariants();
   }

}
