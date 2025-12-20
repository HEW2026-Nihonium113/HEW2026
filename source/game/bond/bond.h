//----------------------------------------------------------------------------
//! @file   bond.h
//! @brief  縁クラス - 2つのエンティティ間の関係を表す
//----------------------------------------------------------------------------
#pragma once

#include "bondable_entity.h"
#include <string>

//----------------------------------------------------------------------------
//! @brief 縁の種類
//----------------------------------------------------------------------------
enum class BondType
{
    Basic,      //!< 基本（攻撃しない）
    Friends,    //!< フレンズ（将来拡張用）
    Love        //!< ラブ（将来拡張用）
};

//----------------------------------------------------------------------------
//! @brief 縁クラス
//! @details 2つのBondableEntity間の関係を表す
//----------------------------------------------------------------------------
class Bond
{
public:
    //! @brief コンストラクタ
    //! @param a 参加者A
    //! @param b 参加者B
    //! @param type 縁の種類
    Bond(BondableEntity a, BondableEntity b, BondType type = BondType::Basic);

    //------------------------------------------------------------------------
    // アクセサ
    //------------------------------------------------------------------------

    //! @brief 参加者Aを取得
    [[nodiscard]] const BondableEntity& GetEntityA() const { return entityA_; }

    //! @brief 参加者Bを取得
    [[nodiscard]] const BondableEntity& GetEntityB() const { return entityB_; }

    //! @brief 縁の種類を取得
    [[nodiscard]] BondType GetType() const { return type_; }

    //! @brief 縁の種類を設定
    void SetType(BondType type) { type_ = type; }

    //------------------------------------------------------------------------
    // 判定
    //------------------------------------------------------------------------

    //! @brief 指定エンティティがこの縁に関与しているか判定
    [[nodiscard]] bool Involves(const BondableEntity& entity) const;

    //! @brief 指定エンティティの縁の相手を取得
    //! @return 相手のエンティティ。関与していない場合はnullptrを含むvariant
    [[nodiscard]] BondableEntity GetOther(const BondableEntity& entity) const;

    //! @brief 2つのエンティティがこの縁で繋がっているか判定
    [[nodiscard]] bool Connects(const BondableEntity& a, const BondableEntity& b) const;

private:
    BondableEntity entityA_;    //!< 参加者A
    BondableEntity entityB_;    //!< 参加者B
    BondType type_;             //!< 縁の種類
};
